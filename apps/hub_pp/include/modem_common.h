#ifndef MODEM_COMMON_H
#define MODEM_COMMON_H

#include <pthread.h>
#include <stdatomic.h>
#include "modemmgr_api.h"

#define NET_PKTSIZE	           1500
#define MAX_MODEMS_COUNT       1
#define DEFAULT_TX_PORT        7777 
#define DEFAULT_RX_PORT        5555
#define MSG_DATA_HEADER_SIZE   3
#define MSG_DATA_SIZE          (NET_PKTSIZE - MSG_DATA_HEADER_SIZE) /* 3 bytes - is 2 bytes id and 1 byte opcode */
#define TS_PKT_PREAMBLE        0xa5a5
#define THR_SLEEP_TIME         10000

typedef uint16_t ts_len_t;
typedef uint16_t ts_preamble_t;
typedef uint32_t ts_header_t;

typedef enum { OP_DATA = 0, OP_ACK, OP_NACK } opcode_t;
typedef enum { ACK_OK = 1, ACK_FAIL } ack_status_t;

typedef struct {
    uint16_t  ack_id;
    uint8_t   opcode;
    uint8_t   status;
    uint16_t  msg_id; 
} ack_t;

typedef struct {
    uint16_t  nack_id;
    uint8_t   opcode;
    uint8_t   status;
    uint16_t  msg_id; 
} nack_t;

typedef struct {
    uint16_t  msg_id;
    uint8_t   opcode;
    uint8_t   data[MSG_DATA_SIZE];
} data_msg_t;

typedef struct {
    uint32_t ip;
    uint32_t txport;
    uint32_t rxport;
    uint32_t pktsize;
    uint32_t modem_id;
    struct circ_queue_t *rxq;
    struct circ_queue_t *txq;
    struct circ_queue_t *ackq;
    pthread_t rxthr;
    pthread_t txthr;
    pthread_mutex_t ack_mutex;
    pthread_cond_t  ack_cond;
    atomic_int      ack_msgid;
    modem_stat_t stat;
} modem_t;

#endif
