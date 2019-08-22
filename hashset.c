#define _GNU_SOURCE
#include "hashset.h"
#include <stdlib.h>
#undef _GNU_SOURCE

hashset_t* hashset_new(void) {
    return calloc(1, sizeof(hashset_t));
}

void hashset_add(hashset_t *set, void *elem) {
    hashentry_t *entry = malloc(sizeof(hashentry_t));
    entry->elem = elem;
    HASH_ADD_PTR(set->head, elem, entry);
}

bool hashset_exist(hashset_t *set, void *elem) {
    hashentry_t *entry = NULL;
    HASH_FIND_PTR(set->head, &elem, entry);
    return entry != NULL;
}

void hashset_remove(hashset_t *set, void *elem) {
    hashentry_t *entry = NULL;
    HASH_FIND_PTR(set->head, &elem, entry);
    if (!entry) return;
    HASH_DEL(set->head, entry);
    free(entry);
}

void hashset_clear(hashset_t *set, hashelem_free_cb freecb) {
    hashentry_t *current_entry = NULL, *temp_entry = NULL;
    HASH_ITER(hh, set->head, current_entry, temp_entry) {
        HASH_DEL(set->head, current_entry);
        freecb(current_entry->elem);
        free(current_entry);
    }
}

void hashset_free(hashset_t *set, hashelem_free_cb freecb) {
    hashset_clear(set, freecb);
    free(set);
}
