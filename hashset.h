#ifndef TCPTUNNEL_HASHSET_H
#define TCPTUNNEL_HASHSET_H

#define _GNU_SOURCE
#include "uthash.h"
#include <stdbool.h>
#undef _GNU_SOURCE

/* hash entry structure typedef */
typedef struct {
    void *elem;
    UT_hash_handle hh;
} hashentry_t;

/* hash set structure typedef */
typedef struct {
    hashentry_t *head;
} hashset_t;

/* hash element freecb typedef */
typedef void (*hashelem_free_cb)(void *elem);

/* create an empty hashset */
hashset_t* hashset_new(void);

/* add an element into hashset */
void hashset_add(hashset_t *set, void *elem);

/* check if a given element exists */
bool hashset_exist(hashset_t *set, void *elem);

/* remove an element from hashset */
void hashset_remove(hashset_t *set, void *elem);

/* remove all elements from hashset */
void hashset_clear(hashset_t *set, hashelem_free_cb freecb);

/* remove all elements from hashset and free `set` */
void hashset_free(hashset_t *set, hashelem_free_cb freecb);

#endif
