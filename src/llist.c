#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "llist.h"
#include "fmeta.h"

/**
 * @relates __llist
 * Initializes a llist.
 * 
 * @param l Target llist to initialize.
 */
void l_init(llist *l) {
    l->head = NULL;
}

/**
 * @relates __llist
 * Insert key, value pair into a llist.
 * 
 * @param l     Target llist.
 * @param key   Key. Pointer to file path stored in corresponding fmeta.
 * @param value Value. Pointer to file metadata.
 */
void l_insert(llist *l, char *key, fmeta *value) {
    lnode_t *new = malloc(sizeof(lnode_t));

    if (new == NULL) {
        perror("malloc");
        return;
    }

    new->key = key;
    new->value = value;

    new->next = l->head;
    l->head = new;
}

/**
 * @relates __llist
 * Returns pointer to file metadata for 
 * file path equal to key. Iterates through nodes
 * until match is found.
 * 
 * @param l     Target llist.
 * @param key   Key to match. Pointer to file path 
 *              stored in corresponding fmeta.
 * @return      Pointer to corresponding file metadata.
 */
fmeta *l_lookup(llist *l, char *key) {
    fmeta *value = NULL;

    lnode_t *curr = l->head;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            value = curr->value;
            break;
        }
        curr = curr->next;
    }

    return value;
}
