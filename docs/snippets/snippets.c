#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "defs.h"

/** Stores file locks for read.txt and empty.txt */
pthread_mutex_t glocks[N_GLOCKS];
/** Linked list of existing target files and their corresponding `fmeta` */
list_t *tracker;



/** 
 * Convert ms to timespec for nanosleep
 * 
 * Based off 
 * https://stackoverflow.com/questions/15024623/convert-milliseconds-to-timespec-for-gnu-port
 */
void ms2ts(struct timespec *ts, unsigned long ms) {
    /** file_server.c:25 Convert ms to s */
    ts->tv_sec = ms / 1000;

    /** file_server.c:28 Convert ms to ns */
    ts->tv_nsec = (ms % 1000) * 1000000L;
}

/**
 * @relates __list_t
 * Initializes a list_t.
 * 
 * @param l Target list_t to initialize.
 */
void l_init(list_t *l) {
    l->head = NULL;
}

/**
 * @relates __list_t
 * Insert key, value pair into a list_t.
 * 
 * @param l     Target list_t.
 * @param key   Key. Pointer to file path stored in corresponding fmeta.
 * @param value Value. Pointer to file metadata.
 */
void l_insert(list_t *l, char *key, fmeta *value) {
    /** file_server.c:51 Dynamically allocate new node */
    lnode_t *new = malloc(sizeof(lnode_t));

    /** file_server.c:54-57 Error handling */
    if (new == NULL) {
        perror("malloc");
        return;
    }

    /** file_server.c:60-64 Build node and insert to linked list */
    new->key = key;
    new->value = value;

    new->next = l->head;
    l->head = new;
}

/**
 * @relates __list_t
 * Returns pointer to file metadata for 
 * file path equal to key. Iterates through nodes
 * until match is found.
 * 
 * @param l     Target list_t.
 * @param key   Key to match. Pointer to file path 
 *              stored in corresponding fmeta.
 * @return      Pointer to corresponding file metadata.
 */
fmeta *l_lookup(list_t *l, char *key) {
    fmeta *value = NULL;

    lnode_t *curr = l->head;
    while (curr) {
        /** file_server.c:84-89 Compare file path as key; save lookup value and exit */
        if (strcmp(curr->key, key) == 0) {
            value = curr->value;
            break;
        }
        curr = curr->next;
    }

    return value;
}

/**
 * Simulate accessing file.
 * 
 * 80% chance of sleeping for 1 second. 
 * 20% chance of sleeping for 6 seconds.
 */
void r_simulate_access() {
    int r = rand() % 100;  /** file_server.c:101 Generate number from 0-99 */
    struct timespec ts;

    if (r < 80)
        ms2ts(&ts, 1 * 1000); /** file_server.c:105 Sleep 1 sec */
    else
        ms2ts(&ts, 6 * 1000); /** file_server.c:107 Sleep 6 secs */

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
    int r = rand(); // No max size

    /** file_server.c:123 Translate random number to range */
    int ms_sleep = (r % (max - min)) + min;

    /** file_server.c:126-128 Convert to ns and sleep */
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
    /** file_server.c:141-146 Writes a 2-input header */
    fprintf(
        to_file, 
        FMT_2HIT,
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
    /** file_server.c:177-179 Read contents per character */
    int copy;
    while ((copy = fgetc(from_file)) != EOF)
        fputc(copy, to_file);

    /** file_server.c:182 Place newline */
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
    FILE *target_file = fopen(path, "w"); /** file_server.c:193 Empty target file */
    fclose(target_file); /** file_server.c:194 Close target file */
}

/**
 * Worker thread function. Contains workload for `write` command.
 * 
 * @param _args     Arguments passed to worker thread. Typecasted back into args_t.
 * @return          Void. Just exists using pthread_exit().
 * 
 * Only requires args.in_lock.
 * Unlocks args.out_lock on exit, allowing next thread to run.
 */ //! [worker_write]
void *worker_write(void *_args) {
    /** file_server.c:208-209 Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** file_server.c:212-237 Critical section for target file */
    pthread_mutex_lock(args->in_lock);


    debug_mode(fprintf(stderr, "[START] %s %s %s\n", cmd->action, cmd->path, cmd->input));
    
    /** file_server.c:218-220 Access target file */
    r_simulate_access();
    FILE *target_file;
    target_file = fopen(cmd->path, "a");

    /** file_server.c:222-226 Error handling */
    if (target_file == NULL) {
        debug_mode(fprintf(stderr, "[ERR] worker_write fopen\n"));
        exit(1);
    }

    /** file_server.c:229-233 Write to target file (with sleep) */
    struct timespec ts;
    ms2ts(&ts, 25 * strlen(cmd->input));
    nanosleep(&ts, NULL);
    fprintf(target_file, "%s", cmd->input);
    fclose(target_file);

    debug_mode(fprintf(stderr, "[END] %s %s %s\n", cmd->action, cmd->path, cmd->input));

    pthread_mutex_unlock(args->out_lock);

    /** file_server.c:240-242 Free unneeded args and struct args */
    free(args->cmd);
    free(args->in_lock);
    free(args);

    pthread_exit(NULL);
}
//! [worker_write]
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
 */ //! [worker_read]
void *worker_read(void *_args) {
    /** file_server.c:260-261 Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** file_server.c:264-311 Critical section for target file */
    pthread_mutex_lock(args->in_lock);

    debug_mode(fprintf(stderr, "[START] %s %s\n", cmd->action, cmd->path));

    /** file_server.c:269-271 Access target file */
    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    /** file_server.c:274-315 Attempt to read target file */
    if (from_file == NULL) {
        /** 
         * file_server.c:280-291 Runs if target file does not exist
         * 
         * - read.txt critical section 
         */
        pthread_mutex_lock(&glocks[READ_GLOCK]);

        /** file_server.c:283 Open read.txt */
        to_file = open_read();

        /** file_server.c:286 Record FILE DNE to read.txt */
        fprintf(to_file, FMT_READ_MISS, cmd->action, cmd->path);

        /** file_server.c:289 Close read.txt */
        fclose(to_file);

        pthread_mutex_unlock(&glocks[READ_GLOCK]);
    } else {
        /** 
         * file_server.c:298-312 
         * - Runs if target file exists
         * - read.txt critical section 
         */
        pthread_mutex_lock(&glocks[READ_GLOCK]);

        /** file_server.c:301 Open read.txt */
        to_file = open_read();
        
        /** file_server.c:304 Write the corresponding record header */
        header_2cmd(to_file, cmd);

        /** file_server.c:307 Append file contents to read.txt */
        fdump(to_file, from_file);

        /** file_server.c:310 Close read.txt */
        fclose(to_file);

        pthread_mutex_unlock(&glocks[READ_GLOCK]);

        /** file_server.c:315 Close target file */
        fclose(from_file);
    }

    debug_mode(fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path));

    pthread_mutex_unlock(args->out_lock); // End critical section

    /** file_server.c:323-325 Free unneeded args and struct args */
    free(args->cmd);
    free(args->in_lock);
    free(args);

    pthread_exit(NULL);
}
//! [worker_read]
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
 */ //! [worker_empty]
void *worker_empty(void *_args) {
    /** file_server.c:343-344 Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** file_server.c:347-409 Critical section for target file */
    pthread_mutex_lock(args->in_lock);

    debug_mode(fprintf(stderr, "[START] %s %s\n", cmd->action, cmd->path));

    /** file_server.c:352-354 Access target file */
    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    /** file_server.c:357-406: Atempt to empty target file */
    if (from_file == NULL) {
        /** 
         * file_server.c:363-374 Runs if target does not exist
         * 
         * - Contains empty.txt critical section 
         */
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);

        /** file_server.c:366 Open empty.txt */
        to_file = open_empty();

        /** file_server.c:369 Record FILE ALR. EMPTY to empty.txt */
        fprintf(to_file, FMT_EMPTY_MISS, cmd->action, cmd->path);

        /** file_server.c:372 Close empty.txt */
        fclose(to_file);

        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);
    } else {
        /** 
         * file_server.c:381-395  Run if target exists
         * 
         * - Contains empty.txt critical section 
         */
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);

        /** file_server.c:384 Open empty.txt */
        to_file = open_empty();

        /** file_server.c:386 Write the corresponding record header to empty.txt */
        header_2cmd(to_file, cmd);

        /** file_server.c:390 Append file contents to empty.txt */
        fdump(to_file, from_file);

        /** file_server.c:393 Close empty.txt */
        fclose(to_file); 

        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);

        /** file_server.c:398-399 Empty target file */
        fclose(from_file);
        empty_file(cmd->path);

        /** 
         * file_server.c:405: Sleep after appending and
         *  emptying (in ms). Random time from 7000ms-10000ms.
         */
        r_sleep_range(7 * 1000, 10 * 1000);
    }
    debug_mode(fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path));

    pthread_mutex_unlock(args->out_lock);

    /** file_server.c:412-414 Free unneeded args and struct args */
    free(args->cmd);
    free(args->in_lock);
    free(args);

    pthread_exit(NULL);
}
//! [worker_empty]
/**
 * @relates     __command
 * Helper function. Reads user into into command struct.
 * 
 * @param cmd   Struct command to be written to,
 * @return      Void.
 */
void get_command(command *cmd) {
    /** 
     * file_server.c:428-436 Reading user input into `inp`
     * - Use fgets to read user command
     * - Use strtok to parse afterwards
     */
    char inp[2 * MAX_INPUT_SIZE + MAX_ACTION_SIZE];

    /** file_server.c:435-436 Read command until newline */
    fgets(inp, sizeof(inp), stdin);
    inp[strcspn(inp, "\n")] = 0;
    /** file_server.c:438-439 Get command action */
    char *ptr = strtok(inp, " ");
    strcpy(cmd->action, ptr);

    /** file_server.c:442-443 Get command path/to/file (aka. target file) */
    ptr = strtok(NULL, " ");
    strcpy(cmd->path, ptr);

    /** file_server.c:446-448 Get command input string */
    ptr = strtok(NULL, "");
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
    /** file_server.c:477-481 Build timestamp using system time */
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    char *cleaned_timestamp = asctime(timeinfo);
    cleaned_timestamp[24] = '\0'; // Remove newline

    /** file_server.c:484 Open commands.txt */
    FILE *commands_file = fopen(CMD_TARGET, CMD_MODE);

    /** file_server.c:487-490 Error handling if commands.txt does not exist */
    if (commands_file == NULL) {
        debug_mode(fprintf(stderr, "[ERR] worker_write fopen\n"));
        exit(1);
    }

    /** file_server.c:493-499 Record based on user input action */
    if (strcmp(cmd->action, "write") == 0) {
        /** file_server.c:495 If recording write action, use 3 inputs */
        fprintf(commands_file, FMT_3CMD, cleaned_timestamp, cmd->action, cmd->path, cmd->input);
    } else {
        /** file_server.c:498 If not recording write action, use 2 inputs */
        fprintf(commands_file, FMT_2CMD, cleaned_timestamp, cmd->action, cmd->path);
    }

    /** file_server.c:502 Close commands.txt */
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
    /** 
     * file_server.c:520-521 
     * - Dynamically allocate new mutex for out_lock; freed later
     * - Initialize mutex
     */
    targs->out_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(targs->out_lock, NULL);

    /** file_server.c:524 Ensure next thread can't run immediately by locking out_lock */
    pthread_mutex_lock(targs->out_lock);

    /** file_server.c:527-528 Deep copy user input stored in cmd to thread args */
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
    /** file_server.c:542 fmeta.path is the only initialization step */
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
    /** file_server.c:556 Overwrite fmeta.recent_lock with new out_lock */
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
     /** file_server.c:569 Use separate pointer for args_t.cmd for brevity */
    command *cmd = targs->cmd;

    /** file_server.c:572-586 Spawn worker thread depending on cmd */
    pthread_t tid;
    if (strcmp(cmd->action, "write") == 0) {
        /** file_server.c:575 If write command, spawn worker_write */
        pthread_create(&tid, NULL, worker_write, targs);
    } else if (strcmp(cmd->action, "read") == 0) {
        /** file_server.c:578 If read command, spawn worker_read */
        pthread_create(&tid, NULL, worker_read, targs);
    } else if (strcmp(cmd->action, "empty") == 0) {
        /** file_server.c:581 If empty command, spawn worker_empty */
        pthread_create(&tid, NULL, worker_empty, targs);
    } else {
        /** file_server.c:584-585 Error handling */
        debug_mode(fprintf(stderr, "[ERR] Invalid `action`\n"));
        exit(1);
    }

    /** file_server.c:589 Detach thread so resources are freed */
    pthread_detach(tid);
}

/**
 * Master thread function.
 * 
 * @param _args     Arguments passed to worker thread. 
 *                  Typecasted back into args_t.
 * @return          Void. Loops indefinitely.
 */ //! [master]
void *master() {
    while (1) {
        /** file_server.c:602-603 Store user input in dynamically allocated struct command */
        command *cmd = malloc(sizeof(command));
        get_command(cmd);

        /** file_server.c:606-607 Build thread args targs */
        args_t *targs = (args_t *) malloc(sizeof(args_t));
        args_init(targs, cmd);

        /** 
         * file_server.c:615-636 Check if target file has been tracked
         * 
         * If found, update `fmeta.recent_lock`.
         * If not found, create `fmeta` and add to tracker.
         */ //! [tracker_check]
        debug_mode(fprintf(stderr, "[METADATA CHECK] %s\n", cmd->path));
        fmeta *fc = l_lookup(tracker, cmd->path);
        if (fc != NULL) {
            /** file_server.c:619-621 Metadata found, update respective fmeta.recent_lock */
            debug_mode(fprintf(stderr, "[METADATA HIT] %s\n", cmd->path));
            /** file_server.c:621 Pass most recent out_lock as new thread's in_lock */ 
            targs->in_lock = fc->recent_lock;
        } else if (fc == NULL) {
            /** file_server.c:624-635 Metadata not found, insert to file tracker */
            debug_mode(fprintf(stderr, "[METADATA ADD] %s\n", cmd->path));

            /** file_server.c:627-628 Dynamically allocate and initialize new in_lock mutex */
            targs->in_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(targs->in_lock, NULL);

            /** file_server.c:631-632 Dynamically allocate and initialize new file metadata fmeta */
            fc = malloc(sizeof(fmeta));
            fmeta_init(fc, cmd);

            /** file_server.c:635 Insert new fmeta to the file tracker */
            l_insert(tracker, fc->path, fc);
        } //! [tracker_check]
        /** file_server.c:638 Whether newly-created or not, update target file's fmeta */
        fmeta_update(fc, targs);

        /** file_server.c:641 Spawn worker thread */
        spawn_worker(targs);

        /** file_server.c:644 Record to commands.txt */
        command_record(cmd);

        /** 
         * file_server.c:650 Free cmd struct
         * Copy of user input already in thread args 
         */
        free(cmd);
    }
}
//! [master]
/**
 * Entrypoint. Initializes global variables and spawns
 * master thread.
 * 
 * @param _args     Arguments passed to worker thread. 
 *                  Typecasted back into args_t.
 * @return          Void. Loops indefinitely.
 */
int main() {
    /** file_server.c:664 Set rng seed */
    srand(time(0));

    /** file_server.c:667-669 Initialize global mutexes */
    int i;
    for (i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    /** file_server.c:672-673 Initialize metadata tracker */
    tracker = malloc(sizeof(list_t));
    l_init(tracker);

    /** file_server.c:676-677 Spawn master thread */
    pthread_t tid;
    pthread_create(&tid, NULL, master, NULL);

    /** file_server.c:680 Wait for master thread */
    pthread_join(tid, NULL);

    return 0;
}
