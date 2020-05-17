#ifndef TCPTUN_SENDQUEUE_H
#define TCPTUN_SENDQUEUE_H

typedef struct sendqueue_t sendqueue_t;

sendqueue_t* sendqueue_new(void);

void sendqueue_push(sendqueue_t *queue, void *element);

void* sendqueue_peek(sendqueue_t *queue);

void* sendqueue_pop(sendqueue_t *queue);

void sendqueue_free(sendqueue_t *queue);

#endif
