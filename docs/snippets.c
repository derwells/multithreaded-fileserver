#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <assert.h>
#include <time.h>

#include "defs.h"

/** Stores file locks for read.txt and empty.txt */ //! [global_vars]
pthread_mutex_t glocks[N_GLOCKS];
/**  
 * Linked list of existing target files and their corresponding `fmeta`. 
 * Indexed using filepath.
 * This is not threadsafe - hence it is non-blocking. 
 * Only the master thread accesses this data structure.
*/
list_t *tracker; //! [global_vars]

/**
 * @relates __list_t
 * Initializes a list_t.
 * 
 * @param l Target list_t to initialize.
 *///! [l_init]
void l_init(list_t *l) {
    l->head = NULL;
}
//! [l_init]
/**
 * @relates __list_t
 * Insert key, value pair into a list_t.
 * 
 * @param l     Target list_t.
 * @param key   Key. Pointer to file path stored in corresponding fmeta.
 * @param value Value. Pointer to file metadata.
 *///! [l_insert]
void l_insert(list_t *l, char *key, fmeta *value) {
    lnode_t *new = malloc(sizeof(lnode_t)); /** Line 41: Dynamically allocate new node */
    /** Lines 43-56: Error handling */
    if (new == NULL) {
        perror("malloc");
        return;
    }

    /** Lines 49-53: Build node and insert to linked list */
    new->key = key;
    new->value = value;

    new->next = l->head;
    l->head = new;
}
//! [l_insert]
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
 */ //! [l_lookup]
fmeta *l_lookup(list_t *l, char *key) {
    fmeta *value = NULL;

    lnode_t *curr = l->head;
    while (curr) {
        /** Lines 73-76: Compare file path as key; save lookup value and exit */
        if (strcmp(curr->key, key) == 0) {
            value = curr->value;
            break;
        }
        curr = curr->next;
    }

    return value;
}
//! [l_lookup]
/**
 * Provided random number generator
 */ //! [rng]
int rng() {
	srand(time(0));
	int r = rand() % 100;

	return r;
}
//! [rng]
/**
 * Simulate accessing file. 
 * 80% chance of sleeping for 1 second. 
 * 20% chance of sleeping for 6 seconds.
 */ //! [r_simulate_access]
void r_simulate_access() {
    int r = rng();  /** Line 99: Generate number from 0-99 */
    if (r < 80) {
        sleep(1);   /** Line 100: Sleep 1 sec */
    } else {
        sleep(6);   /** Line 103: Sleep 6 secs */
    }
}
//! [r_simulate_access]
/**
 * Sleeps for a random amount between min and max.
 * 
 * @param min Minimum sleep time in microseconds.
 * @param max Maximum sleep time in microseconds.
 * @return    Void.
 */ //! [r_sleep_range]
void r_sleep_range(int min, int max) {
    int r = rng();

    /** Line 118: Translate random number to range */
    int sleep_time = (r % (max - min)) + min;
    /** Line 120: Microsecond sleep */
    usleep(sleep_time);
}
//! [r_sleep_range]
/**
 * Generates header in record files. Used for
 * inputs with 2 parameters (read, empty).
 * 
 * @param to_file   Target record file.
 * @param cmd       Issued command to be recorded.
 * @return          Void.
 */ //! [header_2cmd]
void header_2cmd(FILE* to_file, command *cmd) {
    fprintf(
        to_file, 
        FMT_2HIT,
        cmd->action,
        cmd->path
    ); /** Lines 132-137: Writing a 2-input header */
}
//! [header_2cmd]
/**
 * Wrapper for opening read.txt.
 * 
 * @return  FILE pointer to read.txt.
 */ //! [open_read]
FILE *open_read() {
    return fopen(READ_TARGET, READ_MODE);
}
//! [open_read]
/**
 * Wrapper for opening empty.txt.
 * 
 * @return  FILE pointer to empty.txt.
 */ //! [open_empty]
FILE *open_empty() {
    return fopen(EMPTY_TARGET, EMPTY_MODE);
}
//! [open_empty]
/**
 * Append contents of from_file to to_file. Assumes
 * to_file and from_file locks are held.
 * 
 * @param to_file   File to write to.
 * @param to_file   File to read and copy from.
 * @return          Void.
 */ //! [fdump]
void fdump(FILE* to_file, FILE* from_file) {
    /** Lines 168-170: Read contents */
    int copy;
    while ((copy = fgetc(from_file)) != EOF)
        fputc(copy, to_file);
    /** Line 172: Place newline */
    fputc(10, to_file);
}
//! [fdump]
/**
 * Wrapper for emptying a target file.
 * Must hold lock to target_file.
 * 
 * @param path  Filepath of file to empty.
 * @return      Void.
 */ //! [empty_file]
void empty_file(char *path) {
    FILE *target_file = fopen(path, "w");
    fclose(target_file);
}
//! [empty_file]
/**
 * Worker thread function. Contains workload for `write` command.
 * 
 * @param _args     Arguments passed to worker thread. Typecasted back into args_t.
 * @return          Void. Just exists using pthread_exit().
 * 
 * Only requires args.in_lock.
 * 
 * Unlocks args.out_lock on exit, allowing next thread to run.
 */ //! [worker_write]
void *worker_write(void *_args) {
    /** file_server.c:199-200 Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** file_server.c:203-226 Critical section for target file */
    pthread_mutex_lock(args->in_lock);


    debug_mode(fprintf(stderr, "[START] %s %s %s\n", cmd->action, cmd->path, cmd->input));
    
    /** file_server.c:209-211 Access target file */
    r_simulate_access();
    FILE *target_file;
    target_file = fopen(cmd->path, "a");

    /** file_server.c:214-217 Error handling */
    if (target_file == NULL) {
        debug_mode(fprintf(stderr, "[ERR] worker_write fopen\n"));
        exit(1);
    }

    /** file_server.c:220-222 Write to target file (with sleep) */
    usleep(25 * 1000 * strlen(cmd->input));
    fprintf(target_file, "%s", cmd->input);
    fclose(target_file);

    debug_mode(fprintf(stderr, "[END] %s %s %s\n", cmd->action, cmd->path, cmd->input));

    pthread_mutex_unlock(args->out_lock);

    /** file_server.c:229-231 Free unneeded args and struct args */
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
    /** Lines 249-250: Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** Lines 253-298: Critical section for target file */
    pthread_mutex_lock(args->in_lock);

    debug_mode(fprintf(stderr, "[START] %s %s\n", cmd->action, cmd->path));

    /** Lines 258-260: Access target file */
    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    /** Lines 263-294: Attempt to read target file */
    if (from_file == NULL) {
        /** 
         * Lines 269-274: 
         * - Target file does not exists 
         * - read.txt critical section 
         */
        pthread_mutex_lock(&glocks[READ_GLOCK]);
        to_file = open_read(); /** Line 270: Open read.txt */

        fprintf(to_file, FMT_READ_MISS, cmd->action, cmd->path); /** Line 272: Write to read.txt */
        fclose(to_file); /** Line 273: Close read.txt */
        pthread_mutex_unlock(&glocks[READ_GLOCK]);
    } else {
        /** 
         * Lines 281-291: 
         * - Target file exists
         * - read.txt critical section 
         */
        pthread_mutex_lock(&glocks[READ_GLOCK]);
        to_file = open_read(); /** Line 282: Open read.txt */
        
        /** Line 285: Write header */
        header_2cmd(to_file, cmd);

        /** Line 288: Append file contents to read.txt */
        fdump(to_file, from_file);

        fclose(to_file); /** Line 290: Close read.txt */
        pthread_mutex_unlock(&glocks[READ_GLOCK]);

        fclose(from_file); /** Line 293: Close target file */
    }

    debug_mode(fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path));

    pthread_mutex_unlock(args->out_lock); /** Line 298: End critical section */

    /** Lines 301-303: Free unneeded args and struct args */
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
    /** Lines 321-322: Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** Lines 325-322: Critical section for target file */
    pthread_mutex_lock(args->in_lock);

    debug_mode(fprintf(stderr, "[START] %s %s\n", cmd->action, cmd->path));

    /** Lines 330-332: Access target file */
    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    /** Lines 335-: Attempt to empty target file */
    if (from_file == NULL) {
        /** 
         * Lines 341-346: 
         * - If target does not exist
         * - Contains empty.txt critical section 
         */
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);
        to_file = open_empty();

        fprintf(to_file, FMT_EMPTY_MISS, cmd->action, cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);
    } else {
        /** 
         * Lines 353-363: 
         * - If target exists
         * - Contains empty.txt critical section 
         */
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);
        to_file = open_empty();

        /** Line 285: Write header */
        header_2cmd(to_file, cmd);

        /** Line 288: Append file contents to empty.txt */
        fdump(to_file, from_file);

        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);

        /** Lines 366-367: Empty target file */
        fclose(from_file);
        empty_file(cmd->path);

        /** Lines 370: Sleep after appending and emptying */
        r_sleep_range(7*1000000, 10*1000000);
    }
    debug_mode(fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path));

    pthread_mutex_unlock(args->out_lock);

    /** Lines 377-379: Free unneeded args and struct args */
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
 */ //! [get_command]
void get_command(command *cmd) {
    /** file_server.c:396-398 Reading user input into `inp`
     * - Use scanf to read user command
     * - `inp` is parsed afterwards
     */
    char inp[2 * MAX_INPUT_SIZE + MAX_ACTION_SIZE];
    /** file_server.c:397 Read command until newline */
    scanf("%[^\n]%*c", inp);


    /** file_server.c:402-403 Get command action */
    char *ptr = strtok(inp, " ");
    strcpy(cmd->action, ptr);

    /** file_server.c:406-407 Get command path/to/file (aka. target file) */
    ptr = strtok(NULL, " ");
    strcpy(cmd->path, ptr);

    /** file_server.c:410-412 Get command input string */
    ptr = strtok(NULL, "");
    if (strcmp(cmd->action, "write") == 0 && ptr != NULL)
        strcpy(cmd->input, ptr);

    return;
}
//! [get_command]
/**
 * @relates     __command
 * Deep copy of command contents from one command to another.
 * 
 * @param to    Command to copy to.
 * @param from  Command to copy from.
 * @return      Void.
 */ //! [command_copy]
void command_copy(command *to, command *from) {
    strcpy(to->action, from->action);
    strcpy(to->input, from->input);
    strcpy(to->path, from->path);
}
//! [command_copy]
/**
 * @relates     __command
 * Wrapper for writing to commands.txt. Timestamp creation
 * references https://www.cplusplus.com/reference/ctime/localtime/.
 * 
 * @param cmd   Struct command to record.
 * @return      Void.
 */ //! [command_record]
void command_record(command *cmd) {
    /** Lines 441-445: Build timestamp using system time */
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    char *cleaned_timestamp = asctime(timeinfo);
    cleaned_timestamp[24] = '\0';  /** Line 447: Replace newline with null-terminator */

    /** Line 448: Open commands.txt */
    FILE *commands_file = fopen(CMD_TARGET, CMD_MODE);

    /** Lines 451-454: Error handling if commands.txt does not exist */
    if (commands_file == NULL) {
        debug_mode(fprintf(stderr, "[ERR] worker_write fopen\n"));
        exit(1);
    }

    /** Lines 456-463: Error handling if commands.txt does not exist */
    if (strcmp(cmd->action, "write") == 0) {
        /** Line 459: If recording write action, use 3 inputs */
        fprintf(commands_file, FMT_3CMD, cleaned_timestamp, cmd->action, cmd->path, cmd->input);
    } else {
        /** Line 462: If not recording write action, use 2 inputs */
        fprintf(commands_file, FMT_2CMD, cleaned_timestamp, cmd->action, cmd->path);
    }

    /** Line 466: Close commands.txt */
    fclose(commands_file);
}
//! [command_record]
/**
 * @relates     __args_t
 * Initialize thread arguments based on user 
 * input described by command cmd.
 * 
 * @param targs Thread arguments to initialize
 * @param cmd   Struct command to read from.
 * @return      Void.
 */ //! [args_init]
void args_init(args_t *targs, command *cmd) {
    /** Lines 483-484: 
     * Dynamically allocate new mutex for out_lock; freed later
     * Initialize mutex
     */
    targs->out_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(targs->out_lock, NULL);

    /** Line 487: Ensure next thread can't run immediately by locking out_lock */
    pthread_mutex_lock(targs->out_lock);

    /** Lines 490-491: Deep copy user input stored in cmd to thread args */
    targs->cmd = malloc(sizeof(command));
    command_copy(targs->cmd, cmd);
}
//! [args_init]
/**
 * @relates     __fmeta
 * Initialize file metadata based on user 
 * input described by command cmd.
 * 
 * @param fc    File metadata to initialize.
 * @param cmd   Struct command to read from.
 * @return      Void.
 */ //! [fmeta_init]
void fmeta_init(fmeta *fc, command *cmd) {
    /** Line 505: fmeta.path is the only initialization step */
    strcpy(fc->path, cmd->path);
}
//! [fmeta_init]
/**
 * @relates     __fmeta
 * Update file metadata based on built
 * thread arguments.
 * 
 * @param fc    File metadata to update.
 * @param targs Thread arguments to read from.
 * @return      Void.
 */ //! [fmeta_update]
void fmeta_update(fmeta *fc, args_t *targs) {
    /** Line 519: Update fmeta.recent_lock */
    fc->recent_lock = targs->out_lock; 
}
//! [fmeta_update]
/**
 * Wrapper for deciding what thread to spawn
 * 
 * @see         worker_read(), worker_write(), worker_empty()
 * @param targs Thread arguments. Also used for deciding
 *              type of thread.
 * @return      Void.
 */ //! [spawn_worker]
void spawn_worker(args_t *targs) {
     /** Line 532: Use separate pointer for args_t.cmd for brevity */
    command *cmd = targs->cmd;

    /** Lines 535-549: Spawn worker thread depending on cmd */
    pthread_t tid; /** Line 535: Declare tid variable; tids are not saved */
    if (strcmp(cmd->action, "write") == 0) {
        /** Line 538: If write command, spawn worker_write */
        pthread_create(&tid, NULL, worker_write, targs);
    } else if (strcmp(cmd->action, "read") == 0) {
        /** Line 541: If read command, spawn worker_read */
        pthread_create(&tid, NULL, worker_read, targs);
    } else if (strcmp(cmd->action, "empty") == 0) {
        /** Line 544: If empty command, spawn worker_empty */
        pthread_create(&tid, NULL, worker_empty, targs);
    } else {
        /** Lines 547-548: Error handling */
        debug_mode(fprintf(stderr, "[ERR] Invalid `action`\n"));
        exit(1);
    }

    /** Line 552: Detach thread so resources are freed */
    pthread_detach(tid);
}
//! [spawn_worker]
/**
 * Master thread function.
 * 
 * @param _args     Arguments passed to worker thread. 
 *                  Typecasted back into args_t.
 * @return          Void. Loops indefinitely.
 */ //! [master]
void *master() {
    while (1) {
        /** Lines 565-566: Turn input into struct command */
        command *cmd = malloc(sizeof(command));
        get_command(cmd);

        /** Lines 569-570: Build thread args targs */
        args_t *targs = (args_t *) malloc(sizeof(args_t));
        args_init(targs, cmd);

        /** 
         * Lines 578-601: Check if target file has been tracked
         * Check if file metadata exists in tracker.
         * If found, update `fmeta.recent_lock`.
         * If not found, create `fmeta` and add to tracker.
         */ //! [tracker_check]
        debug_mode(fprintf(stderr, "[METADATA CHECK] %s\n", cmd->path));
        fmeta *fc = l_lookup(tracker, cmd->path);
        if (fc != NULL) {
            /** Lines 582-584: Metadata found, update respective fmeta.recent_lock */
            debug_mode(fprintf(stderr, "[METADATA HIT] %s\n", cmd->path));

            targs->in_lock = fc->recent_lock;
        } else if (fc == NULL) {
            /** Line 587-598: Metadata not found, insert to file tracker */
            debug_mode(fprintf(stderr, "[METADATA ADD] %s\n", cmd->path));

            /** Line 590-591: Dynamically allocate and initialize new in_lock mutex */
            targs->in_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(targs->in_lock, NULL);

            /** Line 594-595: Dynamically allocate and initialize new file metadata fmeta */
            fc = malloc(sizeof(fmeta));
            fmeta_init(fc, cmd);

            /** Line 598: Insert new fmeta to the file tracker */
            l_insert(tracker, fc->path, fc);
        }
        /** Line 601: Whether newly-created or not, update target file fmeta */
        fmeta_update(fc, targs);
        //! [tracker_check]
        /** Line 604: Spawn worker thread */
        spawn_worker(targs);

        /** Line 607: Record to commands.txt */
        command_record(cmd);

        /** Line 612: Free cmd struct. 
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
 */ //! [main]
int main() {
    /** Lines 626-628: Initialize global mutexes */
    int i;
    for (i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    /** Lines 631-632: Initialize metadata tracker */
    tracker = malloc(sizeof(list_t));
    l_init(tracker);

    /** Lines 635-636: Spawn master thread */
    pthread_t tid;
    pthread_create(&tid, NULL, master, NULL);

    /** Line 639: Wait for master thread */
    pthread_join(tid, NULL);

    return 0;
}
//! [main]
//! [empty.txt]

//! [empty.txt]
//! [read.txt]

//! [read.txt]
//! [write.txt]

//! [write.txt]