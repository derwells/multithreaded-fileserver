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


typedef struct __command {
    char action[MAX_INP_SIZE + 1];
    char path[MAX_INP_SIZE + 1];
    char input[MAX_INP_SIZE + 1];
} command;

typedef struct __node_t {
    command *cmd;
    pthread_mutex_t *flock;
    struct __node_t *next;
} node_t;

typedef struct __queue_t {
    node_t *head;
    node_t *tail;
    pthread_mutex_t headLock;
    pthread_mutex_t tailLock;
} queue_t;

typedef struct __file_sync {
    pthread_mutex_t flock;
    pthread_cond_t read, write, empty;
    queue_t q;
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