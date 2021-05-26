#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdatomic.h>

#include "logger.h"
#include "circqueue.h"

struct circ_queue_t {
    char       name[QUEUE_NAME_LEN];
    uint8_t    *buf;
    atomic_int head;
    atomic_int tail;
    uint32_t   elem_size;
    uint32_t   elem_count;
};

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(head,tail,size) \
	({int end = (size) - (tail); \
	  int n = ((head) + end) & ((size)-1); \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(head,tail,size) \
	({int end = (size) - 1 - (head); \
	  int n = (end + (tail)) & ((size)-1); \
	  n <= end ? n : end+1;})

int create_queue(uint32_t elem_count, uint32_t elem_size, struct circ_queue_t **q, const char* qname)
{
    if (*q) {
        msgErr("Can't create queue (%s), the 'q' is not NULL!", qname);
        return -1;
    }
    if (elem_size > MAX_ELEM_SIZE || elem_count > MAX_ELEM_COUNT) {
        msgErr("Can't create queue (%s), incorrect params", qname);
        msgErr("elem_count:%u MAX_ELEM_COUNT:%u elem_size:%u MAX_ELEM_SIZE:%u",
                elem_count, MAX_ELEM_COUNT, elem_size, MAX_ELEM_SIZE);
        return -1;
    }
    *q = (struct circ_queue_t*) malloc(sizeof(struct circ_queue_t));
    if (!*q) {
        msgErr("queue (%s) allocation failed. err:%s", qname, strerror(errno));
        return -1;
    }
    (*q)->buf = (uint8_t*) malloc(elem_size * elem_count);
    if (!(*q)->buf) {
        msgErr(" queue's (%s) buf allocation failed. err:%s", qname, strerror(errno));
        return -1;
    }
    atomic_store(&(*q)->head, 0);
    atomic_store(&(*q)->tail, 0);
    (*q)->elem_count = elem_count;
    (*q)->elem_size = elem_size;
    strcpy((*q)->name, qname);
    return 0; 
}

int delete_queue(struct circ_queue_t* q)
{
    if (!q || !q->buf) {
        msgErr("Can't delete queue:%s, the 'q' or 'q->buf' is NULL!", q->name);
        return -1;
    }
    free(q->buf);
    free(q);
    return 0;
}

int queue_elem_count(struct circ_queue_t* q)
{
    return (CIRC_CNT(atomic_load(&q->head), atomic_load(&q->tail), q->elem_count));
}

int deqeue_elem(struct circ_queue_t* q)
{
   unsigned long head = atomic_load(&q->head);
   unsigned long tail = atomic_load(&q->tail);

   if (CIRC_CNT(head, tail, q->elem_count) >= 1) {
       atomic_store(&q->tail, (tail + 1) & (q->elem_count - 1));
//	   msgErr("%s: tail:%d\n", __func__, atomic_load(&q->tail));
   }
   else {
       msgErr("queue (%s) is empty!", q->name); 
       return -1;
   }
   return 0;
}

int get_tail_elem(struct circ_queue_t* q, uint8_t **elem)
{
   unsigned long head = atomic_load(&q->head);
   unsigned long tail = atomic_load(&q->tail);

   if (CIRC_CNT(head, tail, q->elem_count) >= 1) {
       *elem = q->buf + tail * q->elem_size;
   }
   else {
       msgErr("queue (%s) is empty!", q->name);
       return -1;
   }
   return 0;
}

int enqueue_elem(struct circ_queue_t* q, uint8_t *elem, uint32_t elem_size)
{
   unsigned long head = atomic_load(&q->head);
   unsigned long tail = atomic_load(&q->tail);

   if (CIRC_SPACE(head, tail, q->elem_count) >= 1) {
       uint8_t* buf = q->buf + head * q->elem_size;
       memset(buf, 0, q->elem_size);
       memcpy(buf, elem, elem_size);
		
       atomic_store(&q->head, ((head + 1) & (q->elem_count - 1)));
     //  msgErr("%s: head:%d\n", __func__, atomic_load(&q->head));
   }
   else {
       msgErr("queue (%s) is full!", q->name);
       return -1;
   }
   return 0;
}
