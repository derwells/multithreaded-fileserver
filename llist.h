#ifndef LLIST_H
#define LLIST_H

#include <pthread.h>
#include "defs.h"

/** @struct __fmeta
 * @brief File metadata. Key component of hand-over-hand 
 * locking and building worker threads. This keeps track 
 * of the most recent `out_lock` in `fmeta.recent_lock`.
 * 
 * @var __fmeta::recent_lock
 * Most recent out_lock. Used as the next in_lock.
 * @var __fmeta::path
 * File path used to identify file.
 */
typedef struct __fmeta {
    pthread_mutex_t *recent_lock;

    char path[MAX_INPUT_SIZE];
} fmeta;

/** @struct __lnode_t
 * @brief List node. Used to track file metadata 
 * with key, value pair.
 * 
 * @var __lnode_t::key
 * File path used as key.
 * @var __lnode_t::value
 * File metadata used as value.
 * @var __lnode_t::next
 * Next node in linked list.
 */
typedef struct __lnode_t {
    char *key;
    fmeta *value;
    struct __lnode_t *next;
} lnode_t;

/** @struct __llist
 * @brief Non-blocking list struct. Based-off OSTEP.
 * 
 * @var __llist::head
 * Pointer to head of linked list.
 */
typedef struct __llist {
    lnode_t *head;
} llist;


// llist functiosn
void l_init(llist *l);
void l_insert(llist *l, char *key, fmeta *value);
fmeta *l_lookup(llist *l, char *key);

#endif
