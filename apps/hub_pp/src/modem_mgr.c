#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "modem_common.h"
#include "tx_thr.h"
#include "rx_thr.h"

modem_t    g_modems[MAX_MODEMS_COUNT];

int init_modemmgr(const char* modems_cfg_file)
{
    int ind;
    char qname[QUEUE_NAME_LEN];

    logStart("hubpp-modemsmgr", LOG_DEBUG);

    msgInfo("Inits modemsmgr!");
    for (ind = 0; ind < MAX_MODEMS_COUNT; ind++) {
        memset(&g_modems[ind], 0, sizeof(modem_t));
        sprintf(qname, "modem%d-%s", ind, "rxq");
        if (create_queue(MAX_ELEM_COUNT, MAX_ELEM_SIZE, &g_modems[ind].rxq, qname)) {
            msgErr("Can't create RX queue! Exiting");
            return -1;
        }
        sprintf(qname, "modem%d-%s", ind, "txq");
        if (create_queue(MAX_ELEM_COUNT, MAX_ELEM_SIZE, &g_modems[ind].txq, qname)) {
            msgErr("Can't create RX queue! Exiting");
            delete_queue(g_modems[ind].rxq);
            return -1;
        }
        sprintf(qname, "modem%d-%s", ind, "ackq");
        if (create_queue(MAX_ELEM_COUNT, MAX_ELEM_SIZE, &g_modems[ind].ackq, qname)) {
            msgErr("Can't create RX queue! Exiting");
            delete_queue(g_modems[ind].rxq);
            delete_queue(g_modems[ind].txq);
            return -1;
        }
        g_modems[ind].txport = DEFAULT_TX_PORT;
        g_modems[ind].rxport = DEFAULT_RX_PORT;
        g_modems[ind].pktsize = NET_PKTSIZE;
        g_modems[ind].modem_id = ind;
        pthread_mutex_init(&g_modems[ind].ack_mutex, NULL);
        pthread_cond_init(&g_modems[ind].ack_cond, NULL);
    }
    return 0;
}

int start_modemmgr(void)
{
    int ind;

    msgInfo("Starts modemsmgr!");
    for (ind = 0; ind < MAX_MODEMS_COUNT; ind++) {
        if (create_txthr(&g_modems[ind])) {      
            msgErr("Can't create TX thread for modem:%d. Exiting", ind);
            delete_queue(g_modems[ind].rxq);
            delete_queue(g_modems[ind].txq);
            delete_queue(g_modems[ind].ackq);
            return -1;
        }
        if (create_rxthr(&g_modems[ind])) {      
            msgErr("Can't create RX thread for modem:%d. Exiting", ind);
            stop_txthr(&g_modems[ind]);
            delete_queue(g_modems[ind].rxq);
            delete_queue(g_modems[ind].txq);
            delete_queue(g_modems[ind].ackq);
            return -1;
        }
    }
    return 0;
}

int stop_modemmgr(void)
{
    int ind, retval = 0;

    for (ind = 0; ind < MAX_MODEMS_COUNT; ind++) {
        if (stop_rxthr(&g_modems[ind])) {      
            msgErr("Can't stop RX thread of modem:%d. Exiting", ind);
            retval = -1;
        }
        if (stop_txthr(&g_modems[ind])) {      
            msgErr("Can't stop TX thread of modem:%d. Exiting", ind);
            retval = -1;
        }
        delete_queue(g_modems[ind].rxq);
        delete_queue(g_modems[ind].txq);
        delete_queue(g_modems[ind].ackq);
        pthread_mutex_destroy(&g_modems[ind].ack_mutex);
        pthread_cond_destroy(&g_modems[ind].ack_cond);
    }
    return retval;
}

int get_modem_stat(modem_stat_t** stat, int modem_ind)
{
    if (modem_ind >= MAX_MODEMS_COUNT)
        return -1;
    *stat = &g_modems[modem_ind].stat;
    return 0;
}

struct circ_queue_t* get_modem_rxq(int modem_ind)
{
    if (modem_ind >= MAX_MODEMS_COUNT)
        return NULL;
    return g_modems[modem_ind].rxq;
}

struct circ_queue_t* get_modem_txq(int modem_ind)
{
    if (modem_ind >= MAX_MODEMS_COUNT)
        return NULL;
    return g_modems[modem_ind].txq;
}

struct circ_queue_t* get_modem_ackq(int modem_ind)
{
    if (modem_ind >= MAX_MODEMS_COUNT)
        return NULL;
    return g_modems[modem_ind].ackq;
}
