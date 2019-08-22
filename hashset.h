#ifndef TCPTUNNEL_HASHSET_H
#define TCPTUNNEL_HASHSET_H

#define _GNU_SOURCE
#include "uthash.h"
#include <stdbool.h>
#undef _GNU_SOURCE

typedef struct {
    void *elem;
    UT_hash_handle hh;
} hashentry_t;

typedef struct {
    hashentry_t *head;
} hashset_t;

typedef void (*hashelem_free_cb)(void *elem);

hashset_t* hashset_new(void);

void hashset_add(hashset_t *set, void *elem);

bool hashset_exist(hashset_t *set, void *elem);

void hashset_remove(hashset_t *set, void *elem);

void hashset_clear(hashset_t *set, hashelem_free_cb freecb);

void hashset_free(hashset_t *set, hashelem_free_cb freecb);

#endif
