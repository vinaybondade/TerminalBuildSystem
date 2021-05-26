#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>

#include "modemmgr_api.h"
#include "modem_common.h"
#include "circqueue.h"

#define MAX_TS_COUNT    10
#define MAX_TS_LEN      128  
#define START_BYTE_VAL  0xaa
#define TESTS_COUNT     20

typedef struct {
    uint16_t preamble;
    uint16_t len;
    uint8_t  data[MAX_TS_LEN];
} ts_msg_t;

atomic_int g_rx_run, g_tx_run;
int g_rxsock, g_txsock;
struct sockaddr_in g_hubpp_addr;

void sig_handler(int signo)
{
  if (signo == SIGHUP) {
     atomic_store(&g_rx_run, 0);
     atomic_store(&g_tx_run, 0);
     if (close (g_rxsock)) 
        printf("Can't close RX socket err:%s\n", strerror(errno)); 
     if (close(g_txsock))
        printf("Can't close TX socket err:%s\n", strerror(errno)); 
     shutdown(g_rxsock, SHUT_RDWR);
     shutdown(g_txsock, SHUT_RDWR);
     stop_modemmgr();
  }
}

void* ulc_modem_rx(void* args)
{
    struct sockaddr_in srvaddr, clntaddr;
    unsigned int clntlen;
    int enable = 1;
    data_msg_t msg;
    ack_t ack;

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvaddr.sin_port = htons(DEFAULT_TX_PORT); /* port that hubpp transmits to */
    if ((g_rxsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("Can't create socket err:%s\n", strerror(errno));
        return NULL;
    }

	if (setsockopt(g_rxsock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		printf("setsockopt failed... err:%s\n", strerror(errno)); 
	   	return NULL;
	}

    if (bind(g_rxsock, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) < 0) {
        printf("Can't bind Socket err:%s\n", strerror(errno));
        return NULL;
    }
    printf("%s: Running ############\n", __func__);
    ack.opcode = OP_ACK;
    ack.status = ACK_OK;
    while (atomic_load(&g_rx_run)) {
        if (recvfrom(g_rxsock, &msg, sizeof(data_msg_t), 0, (struct sockaddr *) &clntaddr, &clntlen) < 0) {
            printf("Msg receive is failed err:%s. RX Thr is ended", strerror(errno));
            break;
        }
        ack.ack_id = ack.ack_id < USHRT_MAX ? ack.ack_id + 1 : 0;
        ack.msg_id = msg.msg_id;
        if (sendto(g_txsock, &ack, sizeof(ack_t), 0, (struct sockaddr *)&g_hubpp_addr, sizeof(g_hubpp_addr)) < 0) {
            printf("Can't send Data msg. err:%s\n", strerror(errno));
            break;
        }
    }
    return NULL;
}

void* ulc_modem_tx(void* args)
{
    data_msg_t msg;
    int ind;
    uint16_t msgid = 0;
    ts_msg_t *ts = NULL;

    msg.opcode = OP_DATA;
    while(atomic_load(&g_tx_run)) {
        sleep(1);
        for (ind = 0; ind < MAX_TS_COUNT; ind++) {
            msg.msg_id = msgid;
            ts = (ts_msg_t*) (msg.data + (sizeof(ts_msg_t) * ind));
            ts->preamble = TS_PKT_PREAMBLE;
            ts->len = MAX_TS_LEN;
            memset(ts->data, START_BYTE_VAL + ind, MAX_TS_LEN); 
        }

        for (ind = 0; ind < TESTS_COUNT; ind++) {
           if (sendto(g_txsock, &msg, sizeof(data_msg_t), 0, (struct sockaddr *)&g_hubpp_addr, sizeof(g_hubpp_addr)) < 0) {
               printf("ind:%d Can't send Data msg. err:%s\n", ind, strerror(errno));
               break;
           }
        }
        msgid = msgid < USHRT_MAX ? msgid + 1 : 0;
    }
    return NULL;
}

void* fusion_simulation(void* args)
{
    int modem_id = *((int*) args);
    int ind;
    uint16_t tslen;
    uint8_t ts[MAX_TS_LEN], *pdata;
    
    struct circ_queue_t *rxq = get_modem_rxq(modem_id);
    struct circ_queue_t *txq = get_modem_txq(modem_id);

    if (!rxq || !txq ) {
        printf("%s: Can't get RXQ or TXQ of Modem:%d\n", __func__, modem_id);
        return NULL;
    }

    tslen = (MAX_TS_LEN - sizeof(uint16_t)); /* TS Length */
    *((uint16_t*) ts) = tslen;
    pdata = ts + sizeof(uint16_t); /* TS' Data */

    while(atomic_load(&g_tx_run)) {
        while(queue_elem_count(rxq) >= 1) {
            if (deqeue_elem(rxq)) {
                printf("deqeue element is failed!");
                break;  
            }
        }
        for (ind = 0; ind < MAX_TS_COUNT; ind++) {
            memset(pdata, 0xdd + ind, tslen); 
            if (enqueue_elem(txq, ts, MAX_TS_LEN)) {
                printf("%s: Can't enqueue data to TXQ\n", __func__);
                break;
            }
        }
        usleep(100000); /* Sleep 100 millisec */
    }
    return NULL;
}

int main(int argc, char **argv)
{
    modem_stat_t *stat = NULL;
    int modem_ind = 0;
    pthread_t ulc_rx_thr, ulc_tx_thr, fusion_thr;
    struct circ_queue_t *rxq, *txq, *ackq;
    struct hostent *host;

    signal(SIGHUP, sig_handler);
    if (init_modemmgr(NULL) < 0){
        printf("Can't init_modemmgr \n");
        return -1;
    }
    if (start_modemmgr() < 0) {
        printf("Can't start_modemmgr \n");
        return -1;
    }

    atomic_store(&g_rx_run, 1); 
    atomic_store(&g_tx_run, 1);

    if (!(host = gethostbyname("localhost"))) {
        printf("Can't resolve hostname: localhost\n");
        return -1;
    }
    memset(&g_hubpp_addr, 0, sizeof(g_hubpp_addr));
    g_hubpp_addr.sin_family = AF_INET;
    g_hubpp_addr.sin_port = htons(DEFAULT_RX_PORT); /* DEFAULT_RX_PORT - port that hubpp listens on */

    memcpy(&g_hubpp_addr.sin_addr.s_addr, host->h_addr, sizeof(g_hubpp_addr.sin_addr.s_addr));
    if ((g_txsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("Can't create socket \n");
        return -1;
    }
    if(pthread_create(&ulc_rx_thr, NULL, ulc_modem_rx, NULL)) {
        printf("Can't create ULC Modem RX thread\n");
        return -1;
    }
    if(pthread_create(&ulc_tx_thr, NULL, ulc_modem_tx, NULL)) {
        printf("Can't create ULC Modem TX thread\n");
        return -1;
    }
    if(pthread_create(&fusion_thr, NULL, fusion_simulation, &modem_ind)) {
        printf("Can't create Fusion Simulation thread\n");
        return -1;
    }
    sleep(3);

    rxq = get_modem_rxq(modem_ind);
    txq = get_modem_txq(modem_ind);
    ackq = get_modem_ackq(modem_ind);
    while(atomic_load(&g_tx_run)) {
        get_modem_stat(&stat, 0);
        printf("Statistics: #####\n");
        printf("RX: rx_bytes:%" PRIu64 "\n", stat->rx_bytes); 
        printf("RX: rx_pkts:%" PRIu64 "\n", stat->rx_pkts); 
        printf("RX: rx_ts_pkts:%" PRIu64 "\n", stat->rx_ts_pkts); 
        printf("RX: rx_acks:%" PRIu64 "\n", stat->rx_acks); 
        printf("RX: rx_noacks:%" PRIu64 "\n", stat->rx_noacks); 
        printf("TX: tx_bytes:%" PRIu64 "\n", stat->tx_bytes); 
        printf("TX: tx_pkts:%" PRIu64 "\n", stat->tx_pkts); 
        printf("TX: tx_ts_pkts:%" PRIu64 "\n", stat->tx_ts_pkts); 
        printf("TX: tx_acks:%" PRIu64 "\n", stat->tx_acks); 
        printf("TX: tx_noacks:%" PRIu64 "\n", stat->tx_noacks); 
        printf("TX: tx_retransmits:%" PRIu64 "\n", stat->tx_retransmits); 

        printf("Queues info ##\n");
        printf("TXQ: elems count:%d\n", queue_elem_count(txq));
        printf("RXQ: elems count:%d\n", queue_elem_count(rxq));
        printf("ACKQ: elems count:%d\n", queue_elem_count(ackq));
        sleep(1);
    }
    return 0;
}
