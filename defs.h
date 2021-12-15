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
#define CMD_TARGET "commands.txt"
#define CMD_MODE "a"

#define FMT_2HIT "%s %s: "
#define FMT_READ_MISS "%s %s: FILE DNE\n"
#define FMT_EMPTY_MISS "%s %s: FILE ALREADY EMPTY\n"
#define FMT_2CMD "[%s] %s %s\n"
#define FMT_3CMD "[%s] %s %s %s\n"


typedef struct __cmd {
    char action[5 + 1];
    char input[MAX_INP_SIZE + 1];
    char path[MAX_INP_SIZE + 1];
} command;

typedef struct __thread_args {
    pthread_mutex_t *in_lock;
    pthread_mutex_t *out_lock;

    command *cmd;
} args_t;


typedef struct __file_metadata {
    pthread_mutex_t *recent_lock;

    char path[MAX_INP_SIZE + 1];
} fmeta;

typedef struct __lnode_t {
    char *key;
    fmeta *value;
    struct __lnode_t *next;
} lnode_t;

typedef struct __list_t {
    lnode_t *head;
    pthread_mutex_t lock;
} list_t;


#endif