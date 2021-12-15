#ifndef DEFS_H
#define DEFS_H

#include <pthread.h>

#define ACTION 0
#define PATH 1
#define INPUT 2

#define READ_GLOCK 0
#define WRITE_GLOCK 1
#define EMPTY_GLOCK 2

#define MAX_INP_SIZE 50
#define N_GLOCKS 3

#define EMPTY_TARGET "empty.txt"
#define EMPTY_MODE "a"
#define READ_TARGET "read.txt"
#define READ_MODE "a"

#define FMT_2HIT "%s %s: "
#define FMT_3HIT "%s %s %s: "
#define FMT_READ_MISS "%s %s: FILE DNE\n"
#define FMT_EMPTY_MISS "%s %s: FILE ALREADY EMPTY\n"


typedef struct __args_t {
    pthread_mutex_t *in_lock;
    pthread_mutex_t *out_lock;

    char action[MAX_INP_SIZE + 1];
    char input[MAX_INP_SIZE + 1];
    char *path;
} args_t;

typedef struct __file_sync {
    pthread_mutex_t *recent_lock;

    char path[MAX_INP_SIZE + 1];
} fconc;

typedef struct __lnode_t {
    char *key;
    fconc *value;
    struct __lnode_t *next;
} lnode_t;

typedef struct __list_t {
    lnode_t *head;
    pthread_mutex_t lock;
} list_t;


#endif