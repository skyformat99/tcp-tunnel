#define _GNU_SOURCE
#include "sendqueue.h"
#include <stdint.h>
#include <stdlib.h>

#define QSIZE 64

typedef struct arrayqueue_t arrayqueue_t;

struct arrayqueue_t {
    void *        array[QSIZE];
    int           headindex;
    int           tailindex;
    arrayqueue_t *nextqueue;
};

struct sendqueue_t {
    arrayqueue_t *headqueue;
    arrayqueue_t *tailqueue;
};

sendqueue_t* sendqueue_new(void) {
    sendqueue_t *queue = malloc(sizeof(*queue));
    queue->headqueue = queue->tailqueue = NULL;
    return queue;
}

void sendqueue_push(sendqueue_t *queue, void *element) {
    arrayqueue_t *q;
    if (!queue->tailqueue) {
        q = queue->headqueue = queue->tailqueue = malloc(sizeof(*q));
        q->headindex = q->tailindex = -1;
        q->nextqueue = NULL;
    } else {
        q = queue->tailqueue;
        if ((q->headindex == 0 && q->tailindex == QSIZE - 1) || (q->tailindex + 1 == q->headindex)) {
            q = queue->tailqueue = q->nextqueue = malloc(sizeof(*q));
            q->headindex = q->tailindex = -1;
            q->nextqueue = NULL;
        }
    }
    if (q->headindex == -1) q->headindex = 0;
    q->tailindex = (q->tailindex + 1) % QSIZE;
    q->array[q->tailindex] = element;
}

void* sendqueue_peek(sendqueue_t *queue) {
    arrayqueue_t *q = queue->headqueue;
    if (q && q->headindex != 1) {
        return q->array[q->headindex];
    }
    return NULL;
}

void* sendqueue_pop(sendqueue_t *queue) {
    arrayqueue_t *q = queue->headqueue;
    if (q && q->headindex != -1) {
        void *element = q->array[q->headindex];
        if (q->headindex == q->tailindex) {
            q->headindex = q->tailindex = -1;
            if (q->nextqueue) {
                queue->headqueue = q->nextqueue;
                free(q);
            }
        } else {
            q->headindex = (q->headindex + 1) % QSIZE;
        }
        return element;
    }
    return NULL;
}

void sendqueue_free(sendqueue_t *queue) {
    if (queue->headqueue) free(queue->headqueue);
    if (queue->tailqueue) free(queue->tailqueue);
    free(queue);
}
