#ifndef DEFS_H
#define DEFS_H

#include <pthread.h>

#define ACTION 0
#define PATH 1
#define INPUT 2

#define READ_GLOCK 0
#define WRITE_GLOCK 1
#define EMPTY_GLOCK 2

#define MAX_INPUT_SIZE 51
#define MAX_ACTION_SIZE 6
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


/** @struct __command
 * Used to store input in a consistent format.
 * 
 * @var __command::action
 * User action. Only read, write, empty.
 * @var __command::path
 * Target path.
 * @var __command::input
 * User input.
 */
typedef struct __command {
    char action[MAX_ACTION_SIZE];
    char path[MAX_INPUT_SIZE];
    char input[MAX_INPUT_SIZE];
} command;

/** @struct __args_t
 * Arguemnts passed to worker threads.
 * 
 * @var __args_t::in_lock
 * Entry lock for hand-over-hand locking.
 * @var __args_t::out_lock
 * Exit lock for hand-over-hand locking.
 * Unlocked when thread ends.
 * @var __args_t::cmd
 * Pointer to command to execute.
 * Stored separately from command used to read
 * user input.
 */
typedef struct __args_t {
    pthread_mutex_t *in_lock;
    pthread_mutex_t *out_lock;

    command *cmd;
} args_t;

/** @struct __fmeta
 * File metadata. Tracks most recent out_lock.
 * Used in hand-over-hand locking.
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
 * List node. Used to track file metadata 
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

/** @struct __list_t
 * List struct. Based-off OSTEP.
 * 
 * @var __list_t::head
 * Pointer to head of linked list.
 */
typedef struct __list_t {
    lnode_t *head;
} list_t;


#endif
