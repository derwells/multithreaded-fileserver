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
#define N_GLOCKS 2
#define READ_LOCK 0
#define WRITE_LOCK 1

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

void queue_enqeue(queue_t *q, command *cmd) {
    node_t *tmp = malloc(sizeof(node_t));
    assert(tmp != NULL);
    tmp->cmd = cmd;
    tmp->next = NULL;

    pthread_mutex_lock(&q->tailLock);
    q->tail->next = tmp;
    q->tail = tmp;
    pthread_mutex_unlock(&q->tailLock);
}

int queue_dequeue(queue_t *q, command *cmd_out) {
    pthread_mutex_lock(&q->headLock);
    node_t *tmp = q->head;
    node_t *newHead = tmp->next;
    if (newHead == NULL) {
        pthread_mutex_unlock(&q->headLock);
        return -1;
    }
    *cmd_out = *(newHead->cmd);
    q->head = newHead;
    pthread_mutex_unlock(&q->headLock);
    free(tmp);
    return 0;
}

int queue_peek(queue_t *q) {
    int tmp = -1;
    pthread_mutex_lock(&q->headLock);

    if (strcmp(q->head->next->cmd->action, "read") == 0) {
        tmp = READ;
    } else if (strcmp(q->head->next->cmd->action, "write") == 0) {
        tmp = WRITE;
    }
    pthread_mutex_unlock(&q->headLock);
    return tmp;
}

typedef struct __worker_args {
    pthread_mutex_t *flock;
    pthread_cond_t *used;
    queue_t *q;
} wargs;


/** 
 * HASH TABLE
**/
typedef struct __lnode_t {
    int t;
} lnode_t;


/**
    HELPER LIB
**/
void get_input(char **split) {
    char inp[MAX_INP_SIZE*4];
    if(scanf("%[^\n]%*c", inp) == EOF) { while (1) {} } // FIX THIS
    int i = 0;
    char *ptr = strtok(inp, " ");
    while (ptr != NULL) {
        strcpy(split[i++], ptr);
        ptr = strtok(NULL, " ");
    }
}

void simulate_access() {
    // return;
    int prob = (rand() % 100);
    if (prob < 80) {
        sleep(1);
    } else {
        sleep(6);
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
    wargs *args = (wargs *)_args;

    pthread_mutex_lock(args->flock);

    while(queue_peek(args->q) != WRITE)
        pthread_cond_wait(args->used, args->flock);

    command cmd; // check for race
    queue_dequeue(args->q, &cmd);

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

    pthread_cond_signal(args->used);
    pthread_mutex_unlock(args->flock);

    free(args);
}


void *worker_read(void *_args) {
    wargs *args = (wargs *)_args;

    pthread_mutex_lock(args->flock);

    while(queue_peek(args->q) != READ)
        pthread_cond_wait(args->used, args->flock);

    command cmd;
    queue_dequeue(args->q, &cmd);

    FILE *from_file, *to_file;

    strcpy(cmd.path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    fprintf(stderr, "[START] %s %s\n", cmd.action, cmd.path);
    from_file = fopen(cmd.path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", cmd.action, cmd.path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_LOCK]);
    } else {
        pthread_mutex_lock(&glocks[READ_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: ", cmd.action, cmd.path);
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);

        fclose(from_file);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_LOCK]);
    }


    pthread_cond_signal(args->used);
    pthread_mutex_unlock(args->flock);

    free(args);
}

/**
    MAIN
**/
int main() {
    for(int i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    queue_t q1;
    queue_init(&q1);

    pthread_mutex_t flock;
    pthread_mutex_init(&flock, NULL);

    pthread_cond_t used;
    pthread_cond_init(&used, NULL);

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
        queue_enqeue(&q1, cmd);

        wargs *args = malloc(sizeof(wargs));
        args->flock = &flock;
        args->q = &q1;
        args->used = &used;
        pthread_t tid;
        if (strcmp(cmd->action, "write") == 0) {
            pthread_create(&tid, NULL, worker_write, args);
        } else if (strcmp(cmd->action, "read") == 0) {
            pthread_create(&tid, NULL, worker_read, args);
        }

    }

    return 0;
}
