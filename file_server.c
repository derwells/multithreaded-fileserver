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

    // Build node
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

    // Translate random number to range
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
    // Must hold to_file and from_file locks

    // Read contents
    int copy;
    while ((copy = fgetc(from_file)) != EOF)
        fputc(copy, to_file);
    fputc(10, to_file); // Place newline
}

/**
 * Wrapper for emptying a target file.
 * 
 * @param path  Filepath of file to empty.
 * @return      Void.
 */
void empty_file(char *path) {
    // Must hold target_file lock
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
    // Typecast void into args_t
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    // Crtical section start
    pthread_mutex_lock(args->in_lock);


    fprintf(stderr, "[START] %s %s %s\n", cmd->action, cmd->path, cmd->input);
    
    // Access file
    r_simulate_access();
    FILE *target_file;
    target_file = fopen(cmd->path, "a");

    // Error handling
    if (target_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");
        exit(1);
    }

    // Write to file
    sleep(0.025 * strlen(cmd->input));
    fprintf(target_file, "%s", cmd->input);
    fclose(target_file);
    fprintf(stderr, "[END] %s %s %s\n", cmd->action, cmd->path, cmd->input);


    // Critical section end
    pthread_mutex_unlock(args->out_lock);

    // Free
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
    // Typecast void into args_t
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    // Critical section start
    pthread_mutex_lock(args->in_lock);

    fprintf(stderr, "[START] %s %s\n", cmd->action, cmd->path);

    // Access file
    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    // Read file
    if (from_file == NULL) {
        // If file doesn't exist

        pthread_mutex_lock(&glocks[READ_GLOCK]);
        to_file = open_read();

        // Header
        fprintf(to_file, FMT_READ_MISS, cmd->action, cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_GLOCK]);
    } else {
        // File exists
        pthread_mutex_lock(&glocks[READ_GLOCK]);
        to_file = open_read();
        
        // Header
        header_2cmd(to_file, cmd);

        // Read contents
        fdump(to_file, from_file);

        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_GLOCK]);

        fclose(from_file);
    }

    // Critical section end

    fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path);

    // Critical section end
    pthread_mutex_unlock(args->out_lock);

    // Free uneeded args
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
    // Typecast void into args_t
    args_t *args = (args_t *)_args;
    command *cmd = args->cmd;

    // Critical section start
    pthread_mutex_lock(args->in_lock);
    fprintf(stderr, "[START] %s %s\n", cmd->action, cmd->path);

    // Access file
    r_simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(cmd->path, "r");

    // Start manipulating file
    if (from_file == NULL) {
        // If file doesn't exist
    
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);
        to_file = open_empty();

        // Header
        fprintf(to_file, FMT_EMPTY_MISS, cmd->action, cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);
    } else {
        // File exists
        pthread_mutex_lock(&glocks[EMPTY_GLOCK]);
        to_file = open_empty();

        // Place header
        header_2cmd(to_file, cmd);

        // Read contents
        fdump(to_file, from_file);

        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_GLOCK]);

        // Empty contents
        fclose(from_file);
        empty_file(cmd->path);

        // Sleep for 7-10 seconds
        r_sleep_range(7, 10);
    }
    fprintf(stderr, "[END] %s %s\n", cmd->action, cmd->path);

    // Critical section end
    pthread_mutex_unlock(args->out_lock);

    // Free uneeded args
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
    }
    // FIX THIS

    // Get command
    char *ptr = strtok(inp, " ");
    strcpy(cmd->action, ptr);

    // Get path
    ptr = strtok(NULL, " ");
    strcpy(cmd->path, ptr);

    // Get write input
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
    // Deep copy

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
    // Write to commands.txt

    // Build timestamp
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char *cleaned_timestamp = asctime(timeinfo); // Remove newline
    cleaned_timestamp[24] = '\0'; // Check if consistent with older linux kernels

    FILE *commands_file = fopen(CMD_TARGET, CMD_MODE);
    if (commands_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");
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

    // Next thread can't run immediately
    pthread_mutex_lock(targs->out_lock);

    // Deep copy cmd
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

    pthread_t tid; // we don't need to store all tids
    if (strcmp(cmd->action, "write") == 0) {
        pthread_create(&tid, NULL, worker_write, targs);
    } else if (strcmp(cmd->action, "read") == 0) {
        pthread_create(&tid, NULL, worker_read, targs);
    } else if (strcmp(cmd->action, "empty") == 0) {
        pthread_create(&tid, NULL, worker_empty, targs);
    } else {
        perror("[ERR] Invalid `action`");
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
        // Turn input into struct command
        command *cmd = malloc(sizeof(command));
        get_command(cmd);

        // Build thread args targs
        args_t *targs = (args_t *) malloc(sizeof(args_t));
        args_init(targs, cmd);

        // Check if file metadata exists
        fprintf(stderr, "[METADATA CHECK] %s\n", cmd->path);
        fmeta *fc = l_lookup(tracker, cmd->path);
        if (fc != NULL) {
            // Metadata found
            fprintf(stderr, "[METADATA HIT] %s\n", cmd->path);

            targs->in_lock = fc->recent_lock;
        } else if (fc == NULL) {
            // Metadata not found, insert to list
            fprintf(stderr, "[METADATA ADD] %s\n", cmd->path);

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

        // Spawn thread
        spawn_worker(targs);

        // Write to commands.txt
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
    int i;
    for (i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    tracker = malloc(sizeof(list_t));
    l_init(tracker);

    pthread_t tid; // we don't need to store all tids
    pthread_create(&tid, NULL, master, NULL);

    pthread_join(tid, NULL);

    return 0;
}
