#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "defs.h"
#include "llist.h"

// Global locks for read.txt, empty.txt
pthread_mutex_t glocks[N_GLOCKS];

// Linked list of existing target files + metadata
llist *tracker;



/** 
 * Convert ms to timespec for nanosleep
 * 
 * Based off 
 */
void ms2ts(struct timespec *ts, unsigned long ms) {
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000L;
}

/**
 * Simulate accessing file.
 * 
 * 80% chance of sleeping for 1 second. 
 * 20% chance of sleeping for 6 seconds.
 */
void r_simulate_access() {
    int r = rand() % 100;
    struct timespec ts;

    if (r < 80)
        ms2ts(&ts, 1 * 1000);
    else
        ms2ts(&ts, 6 * 1000);

    nanosleep(&ts, NULL);
}

/**
 * Sleeps for a random amount between min and max.
 * 
 * @param min Minimum sleep time in ms.
 * @param max Maximum sleep time in ms.
 * @return    Void.
 */
void r_sleep_range(int min, int max) {
    // No max size
    int r = rand();

    int ms_sleep = (r % (max - min)) + min;

    struct timespec ts;
    ms2ts(&ts, ms_sleep);
    nanosleep(&ts, NULL);
}

/**
 * Generates header in record files. Used for
 * inputs with 2 parameters (read, empty).
 * 
 * @param to_file   Target record file.
 * @param cmd       Issued command to be recorded.
 * @return          Void.
 */
void header_2cmd(FILE* to_file, command *cmd) {
    fprintf(
        to_file, 
        FMT_2LOG,
        cmd->action,
        cmd->path
    );
}

/**
 * Wrapper for opening read.txt.
 * 
 * @return  FILE pointer to read.txt.
 */
FILE *open_read() {
    return fopen(READ_TARGET, READ_MODE);
}

/**
 * Wrapper for opening empty.txt.
 * 
 * @return  FILE pointer to empty.txt.
 */
FILE *open_empty() {
    return fopen(EMPTY_TARGET, EMPTY_MODE);
}

/**
 * Append contents of from_file to to_file. Assumes
 * to_file and from_file locks are held.
 * 
 * @param to_file   File to write to.
 * @param to_file   File to read and copy from.
 * @return          Void.
 */
void fdump(FILE* to_file, FILE* from_file) {
    int copy;
    while ((copy = fgetc(from_file)) != EOF)
        fputc(copy, to_file);

    fputc(10, to_file);
}

/**
 * Wrapper for emptying a target file.
 * Must hold lock to target_file.
 * 
 * @param path  Filepath of file to empty.
 * @return      Void.
 */
void empty_file(char *path) {
    FILE *target_file = fopen(path, "w");
    fclose(target_file);
}

/**
 * Worker thread function. Contains workload for `write` command.
 * 
 * @param _args     Arguments passed to worker thread. Typecasted back into args_t.
 * @return          Void. Just exists using pthread_exit().
 * 
 * Only requires args.in_lock.
 * Unlocks args.out_lock on exit, allowing next thread to run.
 */
void *worker_write(void *_args) {
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    pthread_mutex_lock(args->in_lock);


    debug_print("[START] %s %s %s\n", cmd->action, cmd->path, cmd->input);
    
    r_simulate_access();
    FILE *target_file;
    target_file = fopen(cmd->path, "a");

    if (target_file == NULL) {
        debug_print("[ERR] worker_write fopen\n", NULL);
        exit(1);
    }

    struct timespec ts;
    ms2ts(&ts, 25 * strlen(cmd->input));
    nanosleep(&ts, NULL);
    fprintf(target_file, "%s", cmd->input);
    fclose(target_file);

    debug_print("[END] %s %s %s\n", cmd->action, cmd->path, cmd->input);

    pthread_mutex_unlock(args->out_lock);

    free(args->cmd);
    free(args->in_lock);
    free(args);

    pthread_exit(NULL);
}

/**
 * Worker thread function. Contains workload for `read` command.
 * 
 * @param _args     Arguments passed to worker thread. Typecasted back into args_t.
 * @return          Void. Just exists using pthread_exit().
 * 
 * Initially requires args.in_lock. Needs global file 
 * lock for read.txt (glocks[READ_GLOCK]) when recording.
 * 
 * Unlocks args.out_lock on exit, allowing next thread to run.
 */
void *worker_read(void *_args) {
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    pthread_mutex_lock(args->in_lock);

    debug_print("[START] %s %s\n", cmd->action, cmd->path);

    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ_GLOCK]);

        to_file = open_read();

        fprintf(to_file, FMT_READ_MISS, cmd->action, cmd->path);

        fclose(to_file);

        pthread_mutex_unlock(&glocks[READ_GLOCK]);
    } else {
        pthread_mutex_lock(&glocks[READ_GLOCK]);

        to_file = open_read();
        header_2cmd(to_file, cmd);
        fdump(to_file, from_file);
        fclose(to_file);

        pthread_mutex_unlock(&glocks[READ_GLOCK]);

        fclose(from_file);
    }

    debug_print("[END] %s %s\n", cmd->action, cmd->path);

    pthread_mutex_unlock(args->out_lock);

    free(args->cmd);
    free(args->in_lock);
    free(args);

    pthread_exit(NULL);
}

/**
 * Worker thread function. Contains workload for `empty` command.
 * 
 * @param _args     Arguments passed to worker thread. Typecasted back into args_t.
 * @return          Void. Just exists using pthread_exit().
 * 
 * Initially requires args.in_lock. Needs global file 
 * lock for empty.txt (glocks[EMPTY_GLOCK]) when recording.
 * 
 * Unlocks args.out_lock on exit, allowing next thread to run.
 */
void *worker_empty(void *_args) {
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    pthread_mutex_lock(args->in_lock);

    debug_print("[START] %s %s\n", cmd->action, cmd->path);

    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);

        to_file = open_empty();
        fprintf(to_file, FMT_EMPTY_MISS, cmd->action, cmd->path);
        fclose(to_file);

        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);
    } else {
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);

        to_file = open_empty();
        header_2cmd(to_file, cmd);
        fdump(to_file, from_file);
        fclose(to_file); 

        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);

        fclose(from_file);
        empty_file(cmd->path);

        r_sleep_range(7 * 1000, 10 * 1000);
    }
    debug_print("[END] %s %s\n", cmd->action, cmd->path);

    pthread_mutex_unlock(args->out_lock);

    free(args->cmd);
    free(args->in_lock);
    free(args);

    pthread_exit(NULL);
}

/**
 * @relates     __command
 * Helper function. Reads user into into command struct.
 * 
 * @param cmd   Struct command to be written to,
 * @return      Void.
 */
void get_command(command *cmd) {
    char inp[2 * MAX_INPUT_SIZE + MAX_ACTION_SIZE + 10];

    fgets(inp, sizeof(inp), stdin);
    inp[strcspn(inp, "\n")] = 0;

    char *ptr = strtok(inp, " ");
    strcpy(cmd->action, ptr);

    ptr = strtok(NULL, " ");
    strcpy(cmd->path, ptr);

    ptr = strtok(NULL, "");
    cmd->input[0] = '\0';
    if (strcmp(cmd->action, "write") == 0 && ptr != NULL)
        strcpy(cmd->input, ptr);

    return;
}

/**
 * @relates     __command
 * Deep copy of command contents from one command to another.
 * 
 * @param to    Command to copy to.
 * @param from  Command to copy from.
 * @return      Void.
 */
void command_copy(command *to, command *from) {
    strcpy(to->action, from->action);
    strcpy(to->input, from->input);
    strcpy(to->path, from->path);
}

/**
 * @relates     __command
 * Wrapper for writing to commands.txt. Timestamp creation
 * references https://www.cplusplus.com/reference/ctime/localtime/.
 * 
 * @param cmd   Struct command to record.
 * @return      Void.
 */
void command_record(command *cmd) {
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    char *cleaned_timestamp = asctime(timeinfo);
    cleaned_timestamp[24] = '\0';

    FILE *commands_file = fopen(CMD_TARGET, CMD_MODE);

    if (commands_file == NULL) {
        debug_print("[ERR] worker_write fopen\n", NULL);
        exit(1);
    }

    if (strcmp(cmd->action, "write") == 0) {
        fprintf(
            commands_file,
            FMT_3CMD,
            cleaned_timestamp,
            cmd->action,
            cmd->path,
            cmd->input
        );
    } else {
        fprintf(
            commands_file,
            FMT_2CMD,
            cleaned_timestamp,
            cmd->action,
            cmd->path
        );
    }

    fclose(commands_file);
}

/**
 * @relates     __args_t
 * Initialize thread arguments based on user 
 * input described by command cmd.
 * 
 * @param targs Thread arguments to initialize
 * @param cmd   Struct command to read from.
 * @return      Void.
 */
void args_init(args_t *targs, command *cmd) {
    targs->out_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(targs->out_lock, NULL);

    // Ensure next thread can't run immediately
    pthread_mutex_lock(targs->out_lock);

    // Deep copy user command
    targs->cmd = malloc(sizeof(command));
    command_copy(targs->cmd, cmd);
}

/**
 * @relates     __fmeta
 * Initialize file metadata based on user 
 * input described by command cmd.
 * 
 * @param fc    File metadata to initialize.
 * @param cmd   Struct command to read from.
 * @return      Void.
 */
void fmeta_init(fmeta *fc, command *cmd) {
    strcpy(fc->path, cmd->path);
}

/**
 * @relates     __fmeta
 * Update file metadata based on built
 * thread arguments.
 * 
 * @param fc    File metadata to update.
 * @param targs Thread arguments to read from.
 * @return      Void.
 */
void fmeta_update(fmeta *fc, args_t *targs) {
    fc->recent_lock = targs->out_lock; 
}

/**
 * Wrapper for deciding what thread to spawn
 * 
 * @see         worker_read(), worker_write(), worker_empty()
 * @param targs Thread arguments. Also used for deciding
 *              type of thread.
 * @return      Void.
 */
void spawn_worker(args_t *targs) {
    command *cmd = targs->cmd;

    // Spawn worker thread depending on cmd
    pthread_t tid;
    if (strcmp(cmd->action, "write") == 0) {
        pthread_create(&tid, NULL, worker_write, targs);
    } else if (strcmp(cmd->action, "read") == 0) {
        pthread_create(&tid, NULL, worker_read, targs);
    } else if (strcmp(cmd->action, "empty") == 0) {
        pthread_create(&tid, NULL, worker_empty, targs);
    } else {
        debug_print("[ERR] Invalid `action`\n", NULL);
        exit(1);
    }

    // Detach thread so resources are freed
    pthread_detach(tid);
}

/**
 * Master thread function.
 * 
 * @param _args     Arguments passed to worker thread. 
 *                  Typecasted back into args_t.
 * @return          Void. Loops indefinitely.
 */
void *master() {
    // Master thread loop
    while (1) {
        command *cmd = malloc(sizeof(command));
        get_command(cmd);

        args_t *targs = (args_t *) malloc(sizeof(args_t));
        args_init(targs, cmd);

        // Check if target file has been tracked
        debug_print("[METADATA CHECK] %s\n", cmd->path);
        fmeta *fc = l_lookup(tracker, cmd->path);

        if (fc != NULL) {
            debug_print("[METADATA HIT] %s\n", cmd->path);
            targs->in_lock = fc->recent_lock;
        } else if (fc == NULL) {
            debug_print("[METADATA ADD] %s\n", cmd->path);

            targs->in_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(targs->in_lock, NULL);

            fc = malloc(sizeof(fmeta));
            fmeta_init(fc, cmd);

            l_insert(tracker, fc->path, fc);
        }
        fmeta_update(fc, targs);

        spawn_worker(targs);
        command_record(cmd);

        free(cmd);
    }
}

/**
 * Entrypoint. Initializes global variables and spawns
 * master thread.
 * 
 * @param _args     Arguments passed to worker thread. 
 *                  Typecasted back into args_t.
 * @return          Void. Loops indefinitely.
 */
int main() {
    // Set RNG seed
    srand(time(0));

    int i;
    for (i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    tracker = malloc(sizeof(llist));
    l_init(tracker);

    pthread_t tid;
    pthread_create(&tid, NULL, master, NULL);

    pthread_join(tid, NULL);

    return 0;
}
