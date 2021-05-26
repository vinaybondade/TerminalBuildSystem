#ifndef MODEMMGR_API
#define MODEMMGR_API

#include <stdint.h>
#include "circqueue.h"

typedef struct {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    uint64_t tx_ts_pkts;
    uint64_t rx_ts_pkts;
    uint64_t tx_acks;
    uint64_t rx_acks;
    uint64_t tx_noacks;
    uint64_t rx_noacks;
    uint64_t tx_retransmits;
} modem_stat_t;

int init_modemmgr(const char* modems_cfg_file);
int start_modemmgr(void);
int stop_modemmgr(void);
int get_modem_stat(modem_stat_t** stat, int modem_ind);
struct circ_queue_t* get_modem_rxq(int modem_ind);
struct circ_queue_t* get_modem_txq(int modem_ind);
struct circ_queue_t* get_modem_ackq(int modem_ind);

#endif
