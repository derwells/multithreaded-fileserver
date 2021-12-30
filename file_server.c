#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <assert.h>
#include <time.h>

#include "defs.h"

/** Stores file locks for read.txt and empty.txt */
pthread_mutex_t glocks[N_GLOCKS];
/**  
 * Linked list of existing target files and their corresponding `fmeta`. 
 * Indexed using filepath.
 * This is not threadsafe - hence it is non-blocking. 
 * Only the master thread accesses this data structure.
*/
list_t *tracker;

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
    lnode_t *new = malloc(sizeof(lnode_t));

    if (new == NULL) {
        perror("malloc");
        return;
    }

    /** Lines 49-53: Build node */
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
        /** Line 73: Compare file path */
        if (strcmp(curr->key, key) == 0) {
            value = curr->value;
            break;
        }
        curr = curr->next;
    }

    return value;
}

/**
 * Provided random number generator
 */
int rng() {
	srand(time(0));
	int r = rand() % 100;

	return r;
}

/**
 * Simulate accessing file. 
 * 80% chance of sleeping for 1 second. 
 * 20% chance of sleeping for 6 seconds.
 */
void r_simulate_access() {
    int r = rng();
    if (r < 80) {
        sleep(1);
    } else {
        sleep(6);
    }
}

/**
 * Sleeps for a random amount between min and max.
 * 
 * @param min Minimum sleep time.
 * @param max Maximum sleep time.
 * @return    Void.
 */
void r_sleep_range(int min, int max) {
    int r = rng();

    /** Line 118: Translate random number to range */
    int sleep_time = (r % (max - min)) + min;

    sleep(sleep_time);
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
    /** Lines 168-170: Read contents */
    int copy;
    while ((copy = fgetc(from_file)) != EOF)
        fputc(copy, to_file);
    /** Line 172: Place newline */
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
 * 
 * Unlocks args.out_lock on exit, allowing next thread to run.
 */
void *worker_write(void *_args) {
    /** Lines 199-200: Typecast void into args_t */
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    /** Lines 202-227: Critical section for target file */
    pthread_mutex_lock(args->in_lock);


    debug_mode(fprintf(stderr, "[START] %s %s %s\n", cmd->action, cmd->path, cmd->input));
    
    /** Lines 209-211: Access target file */
    r_simulate_access();
    FILE *target_file;
    target_file = fopen(cmd->path, "a");

    /** Lines 214-217: Error handling */
    if (target_file == NULL) {
        debug_mode(fprintf(stderr, "[ERR] worker_write fopen\n"));
        exit(1);
    }

    /** Lines 220-222: Write to target file (with sleep) */
    sleep(0.025 * strlen(cmd->input));
    fprintf(target_file, "%s", cmd->input);
    fclose(target_file);

    debug_mode(fprintf(stderr, "[END] %s %s %s\n", cmd->action, cmd->path, cmd->input));

    pthread_mutex_unlock(args->out_lock);

    /** Lines 229-231: Free unneeded args and struct args */
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
         * - Target file DNE 
         * - read.txt critical section 
         */
        pthread_mutex_lock(&glocks[READ_GLOCK]);
        to_file = open_read();

        fprintf(to_file, FMT_READ_MISS, cmd->action, cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_GLOCK]);
    } else {
        /** 
         * Lines 281-291: 
         * - Target file exists
         * - read.txt critical section 
         */
        pthread_mutex_lock(&glocks[READ_GLOCK]);
        to_file = open_read();
        
        /** Line 285: Write header */
        header_2cmd(to_file, cmd);

        /** Line 288: Append file contents to read.txt */
        fdump(to_file, from_file);

        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_GLOCK]);

        fclose(from_file);
    }

    debug_mode(fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path));

    pthread_mutex_unlock(args->out_lock);

    /** Lines 301-303: Free unneeded args and struct args */
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
         * - Target DNE
         * - empty.txt critical section 
         */
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);
        to_file = open_empty();

        fprintf(to_file, FMT_EMPTY_MISS, cmd->action, cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);
    } else {
        /** 
         * Lines 353-363: 
         * - Target exists
         * - empty.txt critical section 
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
        r_sleep_range(7, 10);
    }
    debug_mode(fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path));

    pthread_mutex_unlock(args->out_lock);

    /** Lines 377-379: Free unneeded args and struct args */
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
    char inp[2*MAX_INPUT_SIZE + MAX_ACTION_SIZE];
    if (scanf("%[^\n]%*c", inp) == EOF) {
        while (1) {}
    } // FIX THIS

    /** Lines -: Get command action */
    char *ptr = strtok(inp, " ");
    strcpy(cmd->action, ptr);

    /** Lines -: Get command path/to/file (aka. target file) */
    ptr = strtok(NULL, " ");
    strcpy(cmd->path, ptr);

    /** Lines -: Get command input string */
    ptr = strtok(NULL, "");
    if (ptr != NULL)
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
 * Wrapper for writing to commands.txt
 * 
 * @param cmd   Struct command to record.
 * @return      Void.
 */
void command_record(command *cmd) {
    /** Lines -: Build timestamp */
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    // Remove newline
    char *cleaned_timestamp = asctime(timeinfo);
    // Check if consistent with older linux kernels
    cleaned_timestamp[24] = '\0';

    /** Lines -: Write to commands.txt */
    FILE *commands_file = fopen(CMD_TARGET, CMD_MODE);
    if (commands_file == NULL) {
        debug_mode(fprintf(stderr, "[ERR] worker_write fopen\n"));
        exit(1);
    }
    if (strcmp(cmd->action, "write") == 0) {
        fprintf(commands_file, FMT_3CMD, cleaned_timestamp, cmd->action, cmd->path, cmd->input);
    } else {
        fprintf(commands_file, FMT_2CMD, cleaned_timestamp, cmd->action, cmd->path);
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

    /** Line : Ensure next thread can't run immediately */
    pthread_mutex_lock(targs->out_lock);

    /** Lines -: Deep copy user input in cmd */
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

    /** Lines -: Spawn worker thread depending on cmd */
    pthread_t tid; // tids are not stored
    if (strcmp(cmd->action, "write") == 0) {
        pthread_create(&tid, NULL, worker_write, targs);
    } else if (strcmp(cmd->action, "read") == 0) {
        pthread_create(&tid, NULL, worker_read, targs);
    } else if (strcmp(cmd->action, "empty") == 0) {
        pthread_create(&tid, NULL, worker_empty, targs);
    } else {
        /** Lines -: Error handling */
        debug_mode(fprintf(stderr, "[ERR] Invalid `action`\n"));
        exit(1);
    }

    pthread_detach(tid); // Is this ok?
}

/**
 * Master thread function.
 * 
 * @param _args     Arguments passed to worker thread. 
 *                  Typecasted back into args_t.
 * @return          Void. Loops indefinitely.
 */
void *master() {
    while (1) {
        /** Lines -: Turn input into struct command */
        command *cmd = malloc(sizeof(command));
        get_command(cmd);

        /** Lines -: Build thread args targs */
        args_t *targs = (args_t *) malloc(sizeof(args_t));
        args_init(targs, cmd);

        /** 
         * Lines -: 
         *  - Check if file metadata exists in tracker
         *  - If found, update `fmeta.recent_lock`
         *  - If not found, create `fmeta` and add to tracker
         */
        debug_mode(fprintf(stderr, "[METADATA CHECK] %s\n", cmd->path));
        fmeta *fc = l_lookup(tracker, cmd->path);
        if (fc != NULL) {
            // Metadata found
            debug_mode(fprintf(stderr, "[METADATA HIT] %s\n", cmd->path));

            targs->in_lock = fc->recent_lock;
        } else if (fc == NULL) {
            // Metadata not found, insert to list
            debug_mode(fprintf(stderr, "[METADATA ADD] %s\n", cmd->path));

            // Thread can run immediately
            targs->in_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(targs->in_lock, NULL);

            // Metadata per unique file
            fc = malloc(sizeof(fmeta));
            fmeta_init(fc, cmd);

            // Insert to tracker
            l_insert(tracker, fc->path, fc);
        }
        fmeta_update(fc, targs);

        /** Line : Spawn worker thread */
        spawn_worker(targs);

        /** Line : Write to commands.txt */
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
    /** Lines -: Initialize global mutexes */
    int i;
    for (i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    /** Lines -: Initialize metadata tracker */
    tracker = malloc(sizeof(list_t));
    l_init(tracker);

    /** Lines -: Spawn master thread */
    pthread_t tid; // we don't need to store all tids
    pthread_create(&tid, NULL, master, NULL);

    /** Line : Wait for master thread */
    pthread_join(tid, NULL);

    return 0;
}
