#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <assert.h>

#define ACTION 0
#define PATH 1
#define INPUT 2

#define READ 0
#define WRITE 1
#define EMPTY 2

#define MAX_INP_SIZE 50
#define N_GLOCKS 3

pthread_mutex_t glocks[N_GLOCKS];

/** 
 * QUEUE
**/
typedef struct __command {
    char action[MAX_INP_SIZE + 1];
    char path[MAX_INP_SIZE + 1];
    char input[MAX_INP_SIZE + 1];
} command;

typedef struct __node_t {
    command *cmd;
    pthread_mutex_t *flock;
    struct __node_t *next;
} node_t;

typedef struct __queue_t {
    node_t *head;
    node_t *tail;
    pthread_mutex_t headLock;
    pthread_mutex_t tailLock;
} queue_t;


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


typedef struct __file_sync {
    pthread_mutex_t *flock;
    pthread_cond_t *read, *write, *empty;
    queue_t *q;
} fconc;


/** 
 * HASH TABLE
**/
typedef struct __lnode_t {
    char *key;
    fconc *value;
    struct __lnode_t *next;
} lnode_t;

typedef struct __list_t {
    lnode_t *head;
    pthread_mutex_t lock;
} list_t;

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
    HELPER LIB
**/
void get_input(char **split) {
    char inp[MAX_INP_SIZE*4];
    if (scanf("%[^\n]%*c", inp) == EOF) { while (1) {} } // FIX THIS
    int i = 0;
    char *ptr = strtok(inp, " ");
    while (ptr != NULL) {
        strcpy(split[i++], ptr);
        ptr = strtok(NULL, " ");
    }
}

void simulate_access() {
    return;
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
    int val = q_peek(args->q);

    if (val == -1) {
        return;
    }

    if (q_peek(args->q) == READ) {
        pthread_cond_signal(args->read);
    } else if (q_peek(args->q) == WRITE) {
        pthread_cond_signal(args->write);
    } else if (q_peek(args->q) == EMPTY) {
        pthread_cond_signal(args->empty);
    }
}

/**
    COMMANDS LIB
**/
void *worker_write(void *_args) {
    fconc *args = (fconc *)_args;

    pthread_mutex_lock(args->flock);

    command cmd; // check for race
    while(q_peekget(args->q, &cmd, "write") == 1)
        pthread_cond_wait(args->write, args->flock);

    simulate_access();
    fprintf(stderr, "[START] %s: %s\n", cmd.action, cmd.input);

    FILE *target_file;
    strcpy(cmd.path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    target_file = fopen(cmd.path, "a");

    if (target_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");
        exit(1);
    }
    fprintf(target_file, "%s", cmd.input);
    fclose(target_file);

    flex_cond_signal(args);
    pthread_mutex_unlock(args->flock);

    free(args);
}


void *worker_read(void *_args) {
    fconc *args = (fconc *)_args;

    pthread_mutex_lock(args->flock);

    command cmd; // check for race
    while(q_peekget(args->q, &cmd, "read") == 1)
        pthread_cond_wait(args->read, args->flock);


    FILE *from_file, *to_file;

    strcpy(cmd.path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    fprintf(stderr, "[START] %s %s\n", cmd.action, cmd.path);
    from_file = fopen(cmd.path, "r");
    printf("fileno: %d\n", fileno(from_file));

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", cmd.action, cmd.path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ]);
    } else {
        pthread_mutex_lock(&glocks[READ]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
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
    pthread_mutex_unlock(args->flock);

    free(args);
}


void *worker_empty(void *_args) {
    fconc *args = (fconc *)_args;

    pthread_mutex_lock(args->flock);

    command cmd; // check for race
    while(q_peekget(args->q, &cmd, "empty") == 1)
        pthread_cond_wait(args->empty, args->flock);

    FILE *from_file, *to_file;
    fprintf(stderr, "[START] %s %s\n", cmd.action, cmd.path);
    strcpy(cmd.path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    from_file = fopen(cmd.path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", cmd.action, cmd.path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY]);
    } else {
        pthread_mutex_lock(&glocks[EMPTY]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary
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
    pthread_mutex_unlock(args->flock);

    rand_sleep(7, 10);
    free(args);
}


/**
    MAIN
**/
int main() {
    for(int i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);


    // Unique per file
    queue_t q1;
    queue_init(&q1);

    pthread_mutex_t flock;
    pthread_mutex_init(&flock, NULL);

    pthread_cond_t read, write, empty;
    pthread_cond_init(&read, NULL);
    pthread_cond_init(&write, NULL);
    pthread_cond_init(&empty, NULL);

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


        fprintf(stderr, "Enqueue %s %s\n", cmd->action, cmd->input);
        q_put(&q1, cmd);

        fconc *args = malloc(sizeof(fconc));
        args->flock = &flock;
        args->q = &q1;
        args->read = &read;
        args->write = &write;
        args->empty = &empty;
        pthread_t tid; // we don't need to store all tids
        if (strcmp(cmd->action, "write") == 0) {
            pthread_create(&tid, NULL, worker_write, args);
        } else if (strcmp(cmd->action, "read") == 0) {
            pthread_create(&tid, NULL, worker_read, args);
        } else if (strcmp(cmd->action, "empty") == 0) {
            pthread_create(&tid, NULL, worker_empty, args);
        }
    }

    return 0;
}
