#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <limits.h>
#include <sys/time.h>

#include "logger.h"
#include "tx_thr.h"
#include "modem_common.h"

#define MAX_SEND_TRIES         3
#define WAIT_ACK_TIMEOUT       1 /* 1 sec */
#define MAX_RETRANSMIT_COUNT   3
typedef struct {
    struct sockaddr_in addr;
    int sock;
} conn_info_t;

conn_info_t g_conn;
atomic_int g_txrun;

int send_msg(modem_t* modem, uint8_t *msg, uint16_t msglen)
{
    int tries = 0;
    ssize_t bytes = 0;

    while ((bytes = sendto(g_conn.sock, msg, msglen, 0, (struct sockaddr *)&g_conn.addr, sizeof(g_conn.addr))) < 0 
           && tries < MAX_SEND_TRIES) {
        msgErr("SendTo Msg to Modem failed. err:%s. Tries:%d", strerror(errno), tries);
        if ((g_conn.sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
            msgErr("Can't create socket. err:%s", strerror(errno));
        tries++;
   }
   if (tries == MAX_SEND_TRIES)
     return -1;
   modem->stat.tx_bytes += bytes;
   return 0;
}

int handle_rx_acks(modem_t* modem)
{
    int sent = 0;
    ack_t* ack;

    while(queue_elem_count(modem->ackq) >= 1) {
        if(get_tail_elem(modem->ackq, (uint8_t**) &ack)) {
            msgErr("Can't get tail element!");
            break;
        }
        switch(ack->opcode) {
        case OP_ACK:
            if (send_msg(modem, (uint8_t*) ack, sizeof(ack))) {
                msgErr("SendTo ACK to Modem failed. err:%s. Trying to reset socket", strerror(errno));
                return -1;         
            }
            sent++;
            break;
        case OP_NACK:
            msgInfo("NACK message from ACKs queue!");
            break; 
        default:
            msgErr("Unknown message type from ACKs queue!");
        }

        if (deqeue_elem(modem->ackq)) {
            msgErr("Can't deqeue_elem!");
            break;
        } 
    }
    return sent;
}

int prepare_netmsg(modem_t* modem, data_msg_t *msg, uint16_t *msglen)
{
    int retval = -1;
    ts_len_t ts_len = 0;
    uint16_t msg_len = 0;
    uint8_t *ts_msg = NULL, *pdata = msg->data;
  
    while(msg_len < MSG_DATA_SIZE) {
        if(queue_elem_count(modem->txq) >= 1) {
            if (get_tail_elem(modem->txq, &ts_msg)) {
                msgErr("get_tail_elem() is failed!");
                break;  
            }
            ts_len = *((ts_len_t*) ts_msg);
            if ((msg_len + ts_len + sizeof(ts_header_t)) >= MSG_DATA_SIZE)
                break;
            *((ts_preamble_t*) pdata) = TS_PKT_PREAMBLE;
            pdata += sizeof(ts_preamble_t);
            memcpy(pdata, ts_msg, sizeof(ts_len_t) + ts_len);
            pdata += ts_len + sizeof(ts_len_t);
            msg_len += ts_len + sizeof(ts_header_t);
            modem->stat.tx_ts_pkts++;
            if (deqeue_elem(modem->txq)) {
                msgErr("deqeue element is failed!");
                break;  
            }
            retval = 0;
        }
        else { 
            if (!msg_len)
                break;
        } 
    }
    *msglen = msg_len;
    return retval;
}

int wait_for_ack(modem_t* modem, uint16_t msg_id)
{
    struct timespec   ts;
    struct timeval    tp;
    int retval = 0;

    pthread_mutex_lock(&modem->ack_mutex);
    gettimeofday(&tp, NULL);

    /* Convert from timeval to timespec */
    ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += WAIT_ACK_TIMEOUT;
    retval = pthread_cond_timedwait(&modem->ack_cond, &modem->ack_mutex, &ts);
    if (!retval) { /* There is condition signaled */
        if (atomic_load(&modem->ack_msgid) != msg_id) /* But ACK ID is incorrect */
            retval = -1;
    }
    pthread_mutex_unlock(&modem->ack_mutex);
    return retval;
}

void* tx_thr(void* args)
{
    int is_ack = 0, retries = 0;
    modem_t* modem = (modem_t*) args;
    data_msg_t msg;
    uint16_t msglen = 0;

    if (!modem) {
       msgErr("Modem's INFO not supplied!");
       return NULL;
    }

    g_conn.addr.sin_family = AF_INET;
    g_conn.addr.sin_port = htons(modem->txport);
    g_conn.addr.sin_addr.s_addr = modem->ip;
    if ((g_conn.sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        msgErr("Can't create socket. err:%s", strerror(errno));
        return NULL;
    }
    msg.opcode = OP_DATA;
    msg.msg_id = 0;
    while(atomic_load(&g_txrun)) {
        msglen = 0;
        retries = 0;
        if ((is_ack = handle_rx_acks(modem)) < 0) 
            return NULL;
        if (prepare_netmsg(modem, &msg, &msglen)) {
            if (!is_ack)
                usleep(THR_SLEEP_TIME);
        }
        else {
            if (send_msg(modem, (uint8_t*) &msg, msglen + MSG_DATA_HEADER_SIZE))
                return NULL;
            while(wait_for_ack(modem, msg.msg_id) && retries < MAX_RETRANSMIT_COUNT) {
                if (send_msg(modem, (uint8_t*) &msg, msglen + MSG_DATA_HEADER_SIZE))
                    return NULL;
                modem->stat.tx_retransmits++;
                retries++;
            }
            if (retries == MAX_RETRANSMIT_COUNT)
                modem->stat.tx_noacks++;
            else {
                modem->stat.tx_acks++;
                modem->stat.tx_pkts++;
            }
            msg.msg_id = msg.msg_id < USHRT_MAX ? msg.msg_id + 1 : 0;
        }
    }
    return NULL;
}

int create_txthr(modem_t *modem)
{
    if (modem->txport < 0 ) {
       msgErr("TX Thread PORT parameter is incorrect");
       return -1;
    }

    atomic_store(&g_txrun, 1);
    if(pthread_create(&modem->txthr, NULL, tx_thr, modem)) {
        msgErr("Can't create TX thread");
        return -1;
    }
    return 0;
}

int stop_txthr(modem_t *modem)
{
    atomic_store(&g_txrun, 0);
    return 0;
}
