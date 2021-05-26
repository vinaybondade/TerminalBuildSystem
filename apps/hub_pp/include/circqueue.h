#ifndef CIRC_BUF_H
#define CIRC_BUF_H

#include <stdint.h>

#define MAX_ELEM_COUNT	8192
#define MAX_ELEM_SIZE	2048

#ifndef QUEUE_NAME_LEN
#define QUEUE_NAME_LEN  256
#endif

struct circ_queue_t;

/* All functions (except queue_elem_count() 
   returns -1 in case of Failure and 0 in case of Success */

/* The ELEM_COUNT and ELEM_SIZE MUST be POWER of 2 !!!!! */
int create_queue(uint32_t elem_count, uint32_t elem_size, struct circ_queue_t **q, const char* qname);
int delete_queue(struct circ_queue_t* q);

/* Returns elements count in queue or -1 in case of Failure */
int queue_elem_count(struct circ_queue_t* q);

/* Don't deqeue, only return to tail's element pointer */
int get_tail_elem(struct circ_queue_t* q, uint8_t **elem);

/* Tail element already used, you can dequeue it */
int deqeue_elem(struct circ_queue_t* q);

int enqueue_elem(struct circ_queue_t* q, uint8_t *elem, uint32_t elem_size);

#endif /* CIRC_BUF_H  */
