#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <assert.h>

#include "defs.h"


pthread_mutex_t glocks[N_GLOCKS];


/** 
 * LINKED LIST
**/
void l_init(list_t *l) {
    l->head = NULL;
    pthread_mutex_init(&l->lock, NULL);
}

fconc *l_find_or_put(list_t *l, args_t *args, char *path, int *is_found) {
    fconc *value = NULL;

    pthread_mutex_lock(&l->lock);
    lnode_t *curr = l->head;
    while (curr) {
        if (strcmp(curr->key, path) == 0) {
            value = curr->value;
            *(is_found) = 1;
            break;
        }
        curr = curr->next;
    }

    // Not found, insert
    if (value == NULL) {
        lnode_t *new = malloc(sizeof(lnode_t));
        if (new == NULL) {
            perror("malloc");
            return NULL;
        }
        new->value = malloc(sizeof(fconc));
        strcpy(new->value->path, path);
        new->key = new->value->path;

        new->next = l->head;
        l->head = new;

        value = new->value;
    }

    pthread_mutex_unlock(&l->lock);

    return value;
}

/**
 * HELPER LIB
**/
void simulate_access() {
    // return;
    int prob = (rand() % 100);
    if (prob < 80) {
        sleep(1);
    } else {
        // sleep(6);
        sleep(1);
    }
}

void rand_sleep(int min, int max) {
    // return;
    int prob = (rand() % (max - min)) + min;
    sleep(prob);
}

/**
    COMMANDS LIB
**/
void *worker_write(void *_args) {
    args_t *args = (args_t *)_args;

    pthread_mutex_lock(args->in_lock);

    while(args->in_flag == 0)
        pthread_cond_wait(args->in_cond, args->in_lock);

    simulate_access();

    fprintf(stderr, "[START] %s %s %s\n", args->action, args->path, args->input);
    // Write
    FILE *target_file;
    target_file = fopen(args->path, "a");
    if (target_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");
        exit(1);
    }
    fprintf(target_file, "%s", args->input);
    fclose(target_file);
    fprintf(stderr, "[END] %s %s %s\n", args->action, args->path, args->input);

    // Pass
    *args->out_flag = 1;
    pthread_cond_signal(args->out_cond);

    pthread_mutex_unlock(args->out_lock);
    free(args->in_lock);
    free(args->in_cond);
    free(args->in_flag);
    free(args);

    // Free
}


void *worker_read(void *_args) {
    args_t *args = (args_t *)_args;

    pthread_mutex_lock(args->in_lock);

    while(args->in_flag == 0)
        pthread_cond_wait(args->in_cond, args->in_lock);

    fprintf(stderr, "[START] %s %s\n", args->action, args->path);
    // Read
    FILE *from_file, *to_file;
    from_file = fopen(args->path, "r");
    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("read.txt", "a");
        fprintf(to_file, "%s %s: FILE DNE\n", args->action, args->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);
    } else {
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("read.txt", "a");
        fprintf(to_file, "%s %s: ", args->action, args->path);
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);

        fclose(from_file);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);
    }
    fprintf(stderr, "[END] %s %s\n", args->action, args->path);

    // Pass
    *args->out_flag = 1;
    pthread_cond_signal(args->out_cond);

    pthread_mutex_unlock(args->out_lock);
    free(args->in_lock);
    free(args->in_cond);
    free(args->in_flag);
    free(args);
}


void *worker_empty(void *_args) {
    args_t *args = (args_t *)_args;

    pthread_mutex_lock(args->in_lock);

    while(args->in_flag == 0)
        pthread_cond_wait(args->in_cond, args->in_lock);

    fprintf(stderr, "[START] %s %s\n", args->action, args->path);
    // Empty
    FILE *from_file, *to_file;
    from_file = fopen(args->path, "r");
    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("empty.txt", "a");
        fprintf(to_file, "%s %s: FILE DNE\n", args->action, args->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);
    } else {
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("empty.txt", "a");
        int copy;
        if ((copy = fgetc(from_file)) == EOF) {
            fprintf(to_file, "%s %s: FILE DNE\n", args->action, args->path);
        } else {
            fprintf(to_file, "%s %s: ", args->action, args->path);
            fputc(copy, to_file); // sleep per character???
            while ((copy = fgetc(from_file)) != EOF)
                fputc(copy, to_file);

            // newline
            fputc(10, to_file);
        }
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);

        // Empty
        fclose(from_file);
        from_file = fopen(args->path, "w");
        fclose(from_file);
    }
    fprintf(stderr, "[END] %s %s\n", args->action, args->path);

    // Pass
    *args->out_flag = 1;
    pthread_cond_signal(args->out_cond);

    pthread_mutex_unlock(args->out_lock);
    free(args->in_lock);
    free(args->in_cond);
    free(args->in_flag);
    free(args);
    rand_sleep(7, 10);
}


/**
 * MAIN HELPERS
**/
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

/**
 * MAIN
**/
int main() {
    for(int i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);


    list_t *flist = malloc(sizeof(list_t));
    l_init(flist);

    while (1) {
        char **split = malloc(3 * sizeof(char *));
        for (int i = 0; i < 3; i++)
            split[i] = malloc((MAX_INP_SIZE + 1) * sizeof(char)); // free afterwards in thread

        get_input(split);
        args_t *args = malloc(sizeof(args_t));
        strcpy(args->action, split[ACTION]);
        strcpy(args->input, split[INPUT]);

        args->out_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
        args->out_cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
        args->out_flag = malloc(sizeof(int));
        pthread_mutex_init(args->out_lock, NULL);
        pthread_mutex_lock(args->out_lock); // Initialize as locked
        pthread_cond_init(args->out_cond, NULL);
        *args->out_flag = 0;

        // Find file metadata
        int is_found = 0;
        fconc *fc = l_find_or_put(flist, args, split[PATH], &is_found);
        if(is_found) {
            args->in_lock = fc->recent_lock;
            args->in_cond = fc->recent_cond;
            args->in_flag = fc->recent_flag;
            *args->out_flag = 0;
        } else {
            args->path = fc->path;
            args->in_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
            args->in_cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
            args->in_flag = (int *) malloc(sizeof(int));
            pthread_mutex_init(args->in_lock, NULL);
            pthread_cond_init(args->in_cond, NULL);

            *args->in_flag = 1;
        }
        args->path = fc->path;
        fc->recent_lock = args->out_lock;
        fc->recent_cond = args->out_cond;
        fc->recent_flag = args->out_flag;

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

        // Write to commands.txt
        FILE *commands_file = fopen("commands.txt", "a");
        if (commands_file == NULL) {
            fprintf(stderr, "[ERR] worker_write fopen\n");
            exit(1);
        }
        if (strcmp(args->action, "write") == 0) {
            fprintf(commands_file, "%s %s %s\n", args->action, fc->path, args->input);
        } else {
            fprintf(commands_file, "%s %s\n", args->action, fc->path);
        }
        fclose(commands_file);
        // do i put fopen outside?
        // ADD TIMESTAMP
    }

    return 0;
}
