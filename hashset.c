#define _GNU_SOURCE
#include "hashset.h"
#include <stdlib.h>
#undef _GNU_SOURCE

/* create an empty hashset */
hashset_t* hashset_new(void) {
    return calloc(1, sizeof(hashset_t));
}

/* add an element into hashset */
void hashset_add(hashset_t *set, void *elem) {
    hashentry_t *entry = malloc(sizeof(hashentry_t));
    entry->elem = elem;
    HASH_ADD_PTR(set->head, elem, entry);
}

/* check if a given element exists */
bool hashset_exist(hashset_t *set, void *elem) {
    hashentry_t *entry = NULL;
    HASH_FIND_PTR(set->head, &elem, entry);
    return entry != NULL;
}

/* remove an element from hashset */
void hashset_remove(hashset_t *set, void *elem) {
    hashentry_t *entry = NULL;
    HASH_FIND_PTR(set->head, &elem, entry);
    if (!entry) return;
    HASH_DEL(set->head, entry);
    free(entry);
}

/* remove all elements from hashset */
void hashset_clear(hashset_t *set, hashelem_free_cb freecb) {
    hashentry_t *current_entry = NULL, *temp_entry = NULL;
    HASH_ITER(hh, set->head, current_entry, temp_entry) {
        HASH_DEL(set->head, current_entry);
        freecb(current_entry->elem);
        free(current_entry);
    }
}

/* remove all elements from hashset and free `set` */
void hashset_free(hashset_t *set, hashelem_free_cb freecb) {
    hashset_clear(set, freecb);
    free(set);
}
