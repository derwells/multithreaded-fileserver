#ifndef DEFS_H
#define DEFS_H

#include <pthread.h>

#define DEBUG   0
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define N_GLOCKS    2
#define READ_GLOCK  0
#define EMPTY_GLOCK 1

#define MAX_ACTION_SIZE 6
#define MAX_INPUT_SIZE  51

#define EMPTY_TARGET    "empty.txt"
#define EMPTY_MODE      "a"
#define READ_TARGET     "read.txt"
#define READ_MODE       "a"
#define CMD_TARGET      "commands.txt"
#define CMD_MODE        "a"

#define FMT_2LOG        "%s %s: "
#define FMT_READ_MISS   "%s %s: FILE DNE\n"
#define FMT_EMPTY_MISS  "%s %s: FILE ALREADY EMPTY\n"
#define FMT_2CMD        "[%s] %s %s\n"
#define FMT_3CMD        "[%s] %s %s %s\n"


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
 * @brief Arguemnts passed to worker threads.
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

#endif
