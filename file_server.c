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
 * QUEUE
**/
void queue_init(queue_t *q) {
    node_t *tmp = malloc(sizeof(node_t));
    tmp->next = NULL;
    q->head = q->tail = tmp;
    pthread_mutex_init(&q->headLock, NULL);
    pthread_mutex_init(&q->tailLock, NULL);
}


void q_put(queue_t *q, command *cmd) {
    node_t *tmp = malloc(sizeof(node_t));
    assert(tmp != NULL);
    tmp->cmd = cmd;
    tmp->next = NULL;

    pthread_mutex_lock(&q->tailLock);
    q->tail->next = tmp;
    q->tail = tmp;
    pthread_mutex_unlock(&q->tailLock);
}


int q_peekget(queue_t *q, command *cmd_out, char *action) {
    pthread_mutex_lock(&q->headLock);
    node_t *tmp = q->head;
    node_t *newHead = tmp->next;

    if (newHead == NULL) {
        pthread_mutex_unlock(&q->headLock);
        return -1;
    }

    if (strcmp(newHead->cmd->action, action) != 0) {
        pthread_mutex_unlock(&q->headLock);
        return 1;
    }

    *cmd_out = *(newHead->cmd);
    q->head = newHead;
    pthread_mutex_unlock(&q->headLock);

    free(tmp);
    return 0;
}


int q_peek(queue_t *q) {
    pthread_mutex_lock(&q->headLock);
    int ret = -1;
    node_t *top = q->head->next;

    if (top == NULL) {
        pthread_mutex_unlock(&q->headLock);
        return -1;
    }

    char *action = top->cmd->action;
    if (strcmp(action, "read") == 0) {
        ret = READ;
    } else if (strcmp(action, "write") == 0) {
        ret = WRITE;
    } else if (strcmp(action, "empty") == 0) {
        ret = EMPTY;
    }
    pthread_mutex_unlock(&q->headLock);

    return ret;
}

/** 
 * LINKED LIST
**/
void l_init(list_t *l) {
    l->head = NULL;
}

void l_insert(list_t *l, char *key, fconc *value) {
    lnode_t *new = malloc(sizeof(lnode_t));

    if (new == NULL) {
        perror("malloc");
        return;
    }
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

/**
 * FILE METADATA
**/
void fconc_init(fconc *f) {
    queue_init(&f->q);

    pthread_mutex_init(&f->flock, NULL);

    pthread_cond_init(&f->read, NULL);
    pthread_cond_init(&f->write, NULL);
    pthread_cond_init(&f->empty, NULL);
}

/**
    HELPER LIB
**/
void get_input(char **split) {
    char inp[MAX_INP_SIZE * 4];
    if (scanf("%[^\n]%*c", inp) == EOF) { while (1) {} } // FIX THIS
    int nspaces = 0;

    // Get command
    char *ptr = strtok(inp, " ");
    strcpy(split[nspaces++], ptr);

    // Get path
    ptr = strtok(NULL, " ");
    strcpy(split[nspaces++], ptr);

    // Get write input
    ptr = strtok(NULL, "");
    if (ptr != NULL)
        strcpy(split[nspaces], ptr);
}

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

void flex_cond_signal(fconc *args) {
    // only called within file lock
    int val = q_peek(&args->q);

    if (val == -1) {
        return;
    }

    if (val == READ) {
        pthread_cond_signal(&args->read);
    } else if (val == WRITE) {
        pthread_cond_signal(&args->write);
    } else if (val == EMPTY) {
        pthread_cond_signal(&args->empty);
    }
}

/**
    COMMANDS LIB
**/
void *worker_write(void *_args) {
    fconc *args = (fconc *)_args;

    pthread_mutex_lock(&args->flock);

    command cmd; // check for race
    while(q_peekget(&args->q, &cmd, "write") == 1)
        pthread_cond_wait(&args->write, &args->flock);

    simulate_access();
    fprintf(stderr, "[START] %s: %s %s\n", cmd.action, cmd.path, cmd.input);

    FILE *target_file;
    target_file = fopen(cmd.path, "a");
    if (target_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");
        exit(1);
    }
    fprintf(target_file, "%s", cmd.input);
    fclose(target_file);

    flex_cond_signal(args);
    pthread_mutex_unlock(&args->flock);

    fprintf(stderr, "[END] %s: %s\n", cmd.action, cmd.input);
}


void *worker_read(void *_args) {
    fconc *args = (fconc *)_args;

    pthread_mutex_lock(&args->flock);

    command cmd; // check for race
    while(q_peekget(&args->q, &cmd, "read") == 1)
        pthread_cond_wait(&args->read, &args->flock);


    FILE *from_file, *to_file;

    fprintf(stderr, "[START] %s %s\n", cmd.action, cmd.path);
    from_file = fopen(cmd.path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("read.txt", "a");
        fprintf(to_file, "%s %s: FILE DNE\n", cmd.action, cmd.path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);
    } else {
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("read.txt", "a");
        fprintf(to_file, "%s %s: ", cmd.action, cmd.path);
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);

        fclose(from_file);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);
    }

    flex_cond_signal(args);
    pthread_mutex_unlock(&args->flock);

    fprintf(stderr, "[END] %s: %s\n", cmd.action, cmd.path);
}


void *worker_empty(void *_args) {
    fconc *args = (fconc *)_args;

    pthread_mutex_lock(&args->flock);

    command cmd; // check for race
    while(q_peekget(&args->q, &cmd, "empty") == 1)
        pthread_cond_wait(&args->empty, &args->flock);

    FILE *from_file, *to_file;
    fprintf(stderr, "[START] %s %s\n", cmd.action, cmd.path);
    from_file = fopen(cmd.path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("empty.txt", "a");
        fprintf(to_file, "%s %s: FILE DNE\n", cmd.action, cmd.path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);
    } else {
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("empty.txt", "a");
        int copy;
        if ((copy = fgetc(from_file)) == EOF) {
            fprintf(to_file, "%s %s: FILE DNE\n", cmd.action, cmd.path);
        } else {
            fprintf(to_file, "%s %s: ", cmd.action, cmd.path);
            fputc(copy, to_file); // sleep per character???
            while ((copy = fgetc(from_file)) != EOF)
                fputc(copy, to_file);

            // newline
            fputc(10, to_file);
        }
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);

        // empty
        fclose(from_file);
        from_file = fopen(cmd.path, "w");
        fclose(from_file);
    }

    flex_cond_signal(args);
    pthread_mutex_unlock(&args->flock);

    rand_sleep(7, 10);
    fprintf(stderr, "[END] %s: %s\n", cmd.action, cmd.input);
}


/**
    MAIN
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

        command *cmd = malloc(sizeof(command));
        get_input(split);
        strcpy(cmd->action, split[ACTION]);
        strcpy(cmd->path, split[PATH]);
        strcpy(cmd->input, split[INPUT]);

        for (int i = 0; i < 3; i++)
            free(split[i]);
        free(split);

        fconc *fc = l_lookup(flist, cmd->path);
        if (fc == NULL) {
            // Unique per file
            fc = malloc(sizeof(fconc));
            fconc_init(fc);

            l_insert(flist, cmd->path, fc);
        }

        fprintf(stderr, "Enqueue %s %s\n", cmd->action, cmd->input);
        q_put(&fc->q, cmd);

        pthread_t tid; // we don't need to store all tids
        if (strcmp(cmd->action, "write") == 0) {
            pthread_create(&tid, NULL, worker_write, fc);
        } else if (strcmp(cmd->action, "read") == 0) {
            pthread_create(&tid, NULL, worker_read, fc);
        } else if (strcmp(cmd->action, "empty") == 0) {
            pthread_create(&tid, NULL, worker_empty, fc);
        }

        FILE *commands_file = fopen("commands.txt", "a");
        if (commands_file == NULL) {
            fprintf(stderr, "[ERR] worker_write fopen\n");
            exit(1);
        }
        if (strcmp(cmd->action, "write") == 0) {
            fprintf(commands_file, "%s %s %s\n", cmd->action, cmd->path, cmd->input);
        } else {
            fprintf(commands_file, "%s %s\n", cmd->action, cmd->path);
        }
        fclose(commands_file);
        // do i put fopen outside?
        // ADD TIMESTAMP
    }

    return 0;
}
