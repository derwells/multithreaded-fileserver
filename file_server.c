#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <assert.h>
#include <time.h>

#include "defs.h"


pthread_mutex_t glocks[N_GLOCKS];
list_t *flist;

void l_init(list_t *l) {
    l->head = NULL;
}

void l_insert(list_t *l, char *key, fconc *value) {
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

fconc *l_lookup(list_t *l, char *key) {
    fconc *value = NULL;

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


void simulate_access() {
    int prob = (rand() % 100);
    if (prob < 80) {
        sleep(1);
    } else {
        sleep(6);
    }
}

void rand_sleep(int min, int max) {
    int prob = (rand() % (max - min)) + min;
    sleep(prob);
}


void *worker_write(void *_args) {
    // Typecast void into args_t
    args_t *args = (args_t *)_args;

    // Crtical section start
    pthread_mutex_lock(args->in_lock);


    fprintf(stderr, "[START] %s %s %s\n", args->action, args->path, args->input);
    
    // Access file
    simulate_access();
    FILE *target_file;
    target_file = fopen(args->path, "a");

    // Error handling
    if (target_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");
        exit(1);
    }

    // Write to file
    sleep(0.025 * strlen(args->input));
    fprintf(target_file, "%s", args->input);
    fclose(target_file);
    fprintf(stderr, "[END] %s %s %s\n", args->action, args->path, args->input);


    // Critical section end
    pthread_mutex_unlock(args->out_lock);

    // Free
    free(args->in_lock);
    free(args);

}


void *worker_read(void *_args) {
    // Typecast void into args_t
    args_t *args = (args_t *)_args;

    // Critical section start
    pthread_mutex_lock(args->in_lock);

    fprintf(stderr, "[START] %s %s\n", args->action, args->path);

    // Access file
    simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(args->path, "r");

    // Read file
    if (from_file == NULL) {
        // If file doesn't exist

        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("read.txt", "a");

        // Header
        fprintf(to_file, "%s %s: FILE DNE\n", args->action, args->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);
    } else {
        // File exists
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("read.txt", "a");
        
        // Header
        fprintf(to_file, "%s %s: ", args->action, args->path);

        // Read contents
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);
        fputc(10, to_file); // Place newline

        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);

        fclose(from_file);
    }

    // Critical section end

    fprintf(stderr, "[END] %s %s\n", args->action, args->path);

    // Critical section end
    pthread_mutex_unlock(args->out_lock);

    // Free uneeded args
    free(args->in_lock);
    free(args);

}


void *worker_empty(void *_args) {
    // Typecast void into args_t
    args_t *args = (args_t *)_args;

    // Critical section start
    pthread_mutex_lock(args->in_lock);
    fprintf(stderr, "[START] %s %s\n", args->action, args->path);

    // Access file
    simulate_access();
    FILE *from_file, *to_file;
    from_file = fopen(args->path, "r");

    // Start manipulating file
    if (from_file == NULL) {
        // If file doesn't exist
    
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("empty.txt", "a");

        // Header
        fprintf(to_file, "%s %s: FILE ALREADY EMPTY\n", args->action, args->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);
    } else {
        // File exists
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("empty.txt", "a");

        // Read contents
        int copy;
        if ((copy = fgetc(from_file)) == EOF) {
            // File exists but empty (!!!)
            fprintf(to_file, "%s %s: FILE ALREADY EMPTY\n", args->action, args->path);
        } else {
            // File exists and not empty

            // Header
            fprintf(to_file, "%s %s: ", args->action, args->path);

            // Read contents
            fputc(copy, to_file);
            while ((copy = fgetc(from_file)) != EOF)
                fputc(copy, to_file);
            fputc(10, to_file); // Place newline
        }
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);

        // Empty contents
        fclose(from_file);
        from_file = fopen(args->path, "w");
        fclose(from_file);
        
        // Sleep for 7-10 seconds
        rand_sleep(7, 10);
    }
    fprintf(stderr, "[END] %s %s\n", args->action, args->path);

    // Critical section end
    pthread_mutex_unlock(args->out_lock);

    // Free uneeded args
    free(args->in_lock);
    free(args);
}


void get_input(char **split) {
    char inp[MAX_INP_SIZE * 4];
    if (scanf("%[^\n]%*c", inp) == EOF) { while (1) {} } // FIX THIS

    // Get command
    char *ptr = strtok(inp, " ");
    strcpy(split[ACTION], ptr);

    // Get path
    ptr = strtok(NULL, " ");
    strcpy(split[PATH], ptr);

    // Get write input
    ptr = strtok(NULL, "");
    if (ptr != NULL)
        strcpy(split[INPUT], ptr);

    return;
}


int main() {
    for(int i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);


    flist = malloc(sizeof(list_t));
    l_init(flist);

    while (1) {
        char **split = malloc(3 * sizeof(char *));
        for (int i = 0; i < 3; i++)
            split[i] = malloc((MAX_INP_SIZE + 1) * sizeof(char)); // free afterwards in thread

        get_input(split);
        args_t *args = malloc(sizeof(args_t));
        strcpy(args->action, split[ACTION]);
        if (strcmp(args->action, "write") == 0)
            strcpy(args->input, split[INPUT]);


        fprintf(stderr, "[METADATA CHECK] %s\n", split[PATH]);
        args->out_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(args->out_lock, NULL);
        pthread_mutex_lock(args->out_lock); // Initialize as locked

        fconc *fc = l_lookup(flist, split[PATH]);
        if (fc != NULL) {
            // Metadata found
            fprintf(stderr, "[METADATA HIT] %s\n", split[PATH]);
            args->in_lock = fc->recent_lock;
            // Do we copy path to args too? To ensure non-blocking
        } else if (fc == NULL) {
            // Not found, insert to list
            fprintf(stderr, "[METADATA ADD] %s\n", split[PATH]);
            // Build fconc (make function for this)
            fconc *new_fconc = malloc(sizeof(fconc));
            args->in_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(args->in_lock, NULL);

            // Metadata per unique file
            fc = malloc(sizeof(fconc));
            strcpy(fc->path, split[PATH]);
            l_insert(flist, fc->path, fc);
        }
        // Final piece in args
        args->path = fc->path;

        // Update metadata
        fc->recent_lock = args->out_lock;

        // Free split, not needed
        for (int i = 0; i < 3; i++)
            free(split[i]);
        free(split);

        // Spawn thread
        pthread_t tid; // we don't need to store all tids
        if (strcmp(args->action, "write") == 0) {
            pthread_create(&tid, NULL, worker_write, args);
        } else if (strcmp(args->action, "read") == 0) {
            pthread_create(&tid, NULL, worker_read, args);
        } else if (strcmp(args->action, "empty") == 0) {
            pthread_create(&tid, NULL, worker_empty, args);
        }
        pthread_detach(tid); // Is this ok?

        // Write to commands.txt
        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        FILE *commands_file = fopen("commands.txt", "a");
        if (commands_file == NULL) {
            fprintf(stderr, "[ERR] worker_write fopen\n");
            exit(1);
        }
        if (strcmp(args->action, "write") == 0) {
            fprintf(commands_file, "%s %s %s %s\n", args->action, fc->path, args->input, asctime(timeinfo));
        } else {
            fprintf(commands_file, "%s %s %s\n", args->action, fc->path, asctime(timeinfo));
        }
        fclose(commands_file);
        // do i put fopen outside?
        // ADD TIMESTAMP
    }

    return 0;
}
