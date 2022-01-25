#ifndef FMETA_H
#define FMETA_H

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

#endif
