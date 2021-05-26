#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdatomic.h>
#include <limits.h>
#include "logger.h"
#include "rx_thr.h"
#include "modem_common.h"

#define MINIMAL_MSG_SIZE	4 /* 2 bytes - MSG ID, 1 byte - MSG OPCODE, at least 1 byte of data */
atomic_int g_rxrun;
uint16_t g_ackid = 0;

void* rx_thr(void* args)
{
    int sockfd;
    unsigned int clntlen;
    uint8_t msg[NET_PKTSIZE]; 
    uint8_t opcode; 
    struct sockaddr_in srvaddr, clntaddr;
    int enable = 1;
    ssize_t bytes = 0;
    modem_t* modem = (modem_t*) args;


    if (!modem) {
        msgErr("Modem info not supplied!");
        return NULL;
    }
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) < 0 ){
        msgErr("Can't set enable to cancel type");
        return NULL;
    }

    printf("TMPR Waiting for data from sender port #: %d \n", modem->rxport);
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvaddr.sin_port = htons(modem->rxport);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        msgErr("Can't create socket err:%s", strerror(errno));
        return NULL;
    }

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		msgErr("setsockopt failed... err:%s\n", strerror(errno)); 
	   	return NULL;
	}

    if (bind(sockfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) < 0) {
        msgErr("Can't bind Socket err:%s", strerror(errno));
        return NULL;
    }

    while (atomic_load(&g_rxrun)) {
        if ((bytes = recvfrom(sockfd, msg, NET_PKTSIZE, 0, (struct sockaddr *) &clntaddr, &clntlen)) 
                < MINIMAL_MSG_SIZE) {
            msgErr("Msg receive is failed err:%s. RX Thr is ended", strerror(errno));
            break;
        }
        modem->stat.rx_bytes += bytes;
        opcode = *((uint8_t*) (msg + sizeof(uint16_t)));
        switch(opcode) {
            case OP_DATA:
            {
                ts_len_t ts_len = 0;
                ack_t ack;
                data_msg_t *rxmsg = (data_msg_t*) msg;
                uint8_t* pdata = rxmsg->data;
                FILE* fop = NULL; /* for debug */
                uint32_t parsed_bytes = 0;

				fop = fopen("/tmp/hub.bin", "w"); /* for debug */
                modem->stat.rx_pkts++;
                while (*((ts_preamble_t*) pdata) == TS_PKT_PREAMBLE) {
                    parsed_bytes += sizeof(ts_preamble_t);
                    pdata += sizeof(ts_preamble_t);
                    if (parsed_bytes >= MSG_DATA_SIZE)
                       break;

                    ts_len = *((ts_len_t*) pdata);
                    parsed_bytes += (sizeof(ts_len_t) + ts_len);
                    if (parsed_bytes >= MSG_DATA_SIZE)
                       break;

                    if (enqueue_elem(modem->rxq, pdata, ts_len + sizeof(ts_len_t))) 
                        msgErr("Can't add TS of RXMsg to rxqueue is failed err:%s", strerror(errno));
                    fwrite(pdata, ts_len + sizeof(ts_len_t), 1, fop); /* For debug */
                    pdata += (sizeof(ts_len_t) + ts_len);
                    modem->stat.rx_ts_pkts++;
                }
                fclose(fop); /* For debug */

                g_ackid = g_ackid < USHRT_MAX ? g_ackid + 1 : 0;
                ack.opcode = OP_ACK;
                ack.ack_id = g_ackid;
                ack.msg_id = rxmsg->msg_id;
                ack.status = ACK_OK;
                if (enqueue_elem(modem->ackq, (uint8_t*) &ack, sizeof(ack_t))) 
                     msgErr("Can't add ACK of RXMsg to ack-queue is failed err:%s", strerror(errno));
                 modem->stat.rx_acks++;
                break;
            }
            case OP_ACK:
            {
                ack_t* ackmsg = (ack_t*) msg;

                //msgDbg("ACK msg arrived");
                pthread_mutex_lock(&modem->ack_mutex);
                atomic_store(&modem->ack_msgid, ackmsg->msg_id);
                pthread_cond_signal(&modem->ack_cond);
                pthread_mutex_unlock(&modem->ack_mutex);
                break;
            }
            case OP_NACK:
            {
                nack_t* nackmsg = (nack_t*) msg;

                msgDbg("NACK msg arrived");
                modem->stat.rx_noacks++;
                if (enqueue_elem(modem->rxq, (uint8_t*) nackmsg, sizeof(nack_t))) 
                    msgErr("Msg  NACK receive is failed err:%s", strerror(errno));
                modem->stat.tx_noacks++;
                break;
            }
            default:
                msgErr("Unknown message type from RXMsg!");
            }
    }
    if (close (sockfd) < 0) 
        msgErr("Can't close socket err:%s", strerror(errno));
    return NULL;
}

int create_rxthr(modem_t *modem)
{
    if (modem->rxport < 0 ) {
       msgErr("RX Thread PORT parameter is incorrect");
       return -1;
    }
    
    atomic_store(&g_rxrun, 1);
    if(pthread_create(&modem->rxthr, NULL, rx_thr, modem)) {
        msgErr("Can't create RX thread");
        return -1;
    }
    return 0;
}

int stop_rxthr(modem_t *modem)
{
    if (!atomic_load(&g_rxrun)) {
        msgErr("RX Thr already stopped!");
        return 0;
    }
 
    atomic_store(&g_rxrun, 0);
    if(pthread_cancel(modem->rxthr) < 0) {
        msgErr("Can't cancel thread");
        return -1;
    }
    return 0;
}
