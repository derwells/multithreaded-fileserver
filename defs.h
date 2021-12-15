#ifndef DEFS_H
#define DEFS_H

#include <pthread.h>

#define ACTION 0
#define PATH 1
#define INPUT 2

#define READ 0
#define WRITE 1
#define EMPTY 2

#define MAX_INP_SIZE 50
#define N_GLOCKS 3


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