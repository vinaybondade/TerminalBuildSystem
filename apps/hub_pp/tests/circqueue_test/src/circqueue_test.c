#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "circqueue.h"

#define ELEM_SIZE	1024
#define ELEM_COUNT	8	

int main(int argc, char **argv)
{
    struct circ_queue_t *q = NULL;
    int ind = 0;
    uint8_t* buf = NULL;

    if (create_queue(ELEM_COUNT, ELEM_SIZE, &q))
        return -1;

    for (ind = 0; ind < argc; ind++) {
        if (enqueue_elem(q, (uint8_t*) argv[ind], strlen(argv[ind])))
           break;
       	deqeue_elem(q, &buf);
       	printf("Elem[%d]: %s\n", ind, buf);
    }
    printf("Elements in queue:%d\n", queue_elem_count(q));

    ind = 0;
    while(queue_elem_count(q)) {
       deqeue_elem(q, &buf);
       printf("Elem[%d]: %s\n", ind, buf);
       ind++;
    }
    return 0;
}
