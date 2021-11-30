#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>

#define ACTION 0
#define PATH 1
#define INPUT 2
#define MAX_INP_SIZE 50
#define N_GLOCKS 2
#define READ_LOCK 0
#define EMPTY_LOCK 1

pthread_mutex_t glocks[N_GLOCKS];

struct file_locks {
    pthread_mutex_t mutex;
    pthread_cond_t *recent_cond;
}file_locks;

struct command {
    char action[MAX_INP_SIZE + 1];
    char path[MAX_INP_SIZE + 1];
    char input[MAX_INP_SIZE + 1];
}command;

struct enter_exit {
    pthread_cond_t *enter;
    pthread_cond_t *exit;
}enter_exit;

struct worker_args {
    struct command *cmd;
    struct file_locks *fls;
    struct enter_exit *enter_exit;
}worker_args;

/**
    HELPER LIB
**/
void get_input(char *inp, char **split) {
    scanf("%[^\n]%*c", inp);

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
    struct worker_args *args = (struct worker_args *) _args;
    pthread_mutex_lock(&(args->fls->mutex));
    while (args->enter_exit->enter  NULL)
        pthread_cond_wait(args->enter_exit->enter, &(args->fls->mutex));


    simulate_access();

    FILE *target_file;
    strcpy(args->cmd->path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    fprintf(stderr, "[START] %s %s\n", args->cmd->action, args->cmd->input);
    target_file = fopen(args->cmd->path, "a");

    if (target_file == NULL) {
        fprintf(stderr, "[ERR] worker_write fopen\n");   
        exit(1);
    }

    fprintf(target_file, "%s", args->cmd->input);
    fclose(target_file);

    pthread_cond_broadcast(args->enter_exit->exit);
    pthread_mutex_unlock(&(args->fls->mutex));
    // free(args->cmd);
}


void *worker_read(void *_args) {
    struct worker_args *args = (struct worker_args *) _args;
    pthread_mutex_lock(&(args->fls->mutex));
    while (args->enter_exit->enter == NULL) {
        pthread_cond_wait(args->enter_exit->enter, &(args->fls->mutex));
    }
    simulate_access();

    FILE *from_file, *to_file;

    strcpy(args->cmd->path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    fprintf(stderr, "[START] %s %s\n", args->cmd->action, args->cmd->path);
    from_file = fopen(args->cmd->path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd->action, args->cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_LOCK]);
    } else {
        pthread_mutex_lock(&glocks[READ_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: ", args->cmd->action, args->cmd->path);
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);

        fclose(from_file);
        pthread_mutex_unlock(&glocks[READ_LOCK]);
    }

    pthread_cond_broadcast(args->enter_exit->exit);

    pthread_mutex_unlock(&(args->fls->mutex));
    // free(args->cmd);
}


void *worker_empty(void *_args) {
    struct worker_args *args = (struct worker_args *) _args;

    simulate_access();

    FILE *from_file, *to_file;

    strcpy(args->cmd->path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    pthread_mutex_lock(&(args->fls->mutex));
    from_file = fopen(args->cmd->path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[EMPTY_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd->action, args->cmd->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_LOCK]);
    } else {
        pthread_mutex_lock(&glocks[EMPTY_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary
        int copy;
        if ((copy = fgetc(from_file)) == EOF) {
            fprintf(to_file, "%s %s: FILE DNE\n", args->cmd->action, args->cmd->path);
        } else {
            fprintf(to_file, "%s %s: ", args->cmd->action, args->cmd->path);
            fputc(copy, to_file); // sleep per character???
            while ((copy = fgetc(from_file)) != EOF)
                fputc(copy, to_file);

            // newline
            fputc(10, to_file);
        }
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_LOCK]);

        // empty
        fclose(from_file);
        from_file = fopen(args->cmd->path, "w");
        fclose(from_file);
    }
    pthread_mutex_unlock(&(args->fls->mutex));

    rand_sleep(7, 10);
    // free(args->cmd);
}



/**
    MAIN
**/
int main() {
    for(int i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    int iter = 0;
    char inp[] = "";
    while (1) {
        char **split = malloc(3 * sizeof(char *));
        for (int i = 0; i < 3; i++)
            split[i] = malloc((MAX_INP_SIZE + 1) * sizeof(char)); // free afterwards in thread


        get_input(inp, split);

        struct worker_args *args = (struct worker_args *)malloc(sizeof(worker_args));
        args->cmd = (struct command *)malloc(sizeof(command));
        strcpy(args->cmd->action, split[ACTION]);
        strcpy(args->cmd->path, split[PATH]);
        strcpy(args->cmd->input, split[INPUT]);

        for (int i = 0; i < 3; i++)
            free(split[i]);
        free(split);

        struct file_locks *fls;
        if (iter == 0) {
            fls = (struct file_locks *)malloc(sizeof(struct file_locks));
            pthread_mutex_init((pthread_mutex_t *)&(fls->mutex), NULL);
        
            fls->recent_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
            pthread_cond_init(fls->recent_cond, NULL);
            args->fls = fls;
            args->enter_exit = (struct enter_exit *)malloc(sizeof(enter_exit));
            args->enter_exit->exit = args->fls->recent_cond;
            args->enter_exit->enter = NULL;
        }

        pthread_t tid;
        if (strcmp(args->cmd->action, "write") == 0) {
            if (iter > 0) {
                args->fls = fls;
                args->enter_exit = (struct enter_exit *)malloc(sizeof(enter_exit));
                args->enter_exit->enter = args->fls->recent_cond;

                args->fls->recent_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
                pthread_cond_init(args->fls->recent_cond, NULL);
                args->enter_exit->exit = args->fls->recent_cond;
            }
            pthread_create(&tid, NULL, worker_write, args);
        } else if (strcmp(args->cmd->action, "read") == 0) {
            if (iter > 0) {
                args->fls = fls;
                args->enter_exit = (struct enter_exit *)malloc(sizeof(enter_exit));
                args->enter_exit->enter = args->fls->recent_cond;
                args->enter_exit->exit = args->fls->recent_cond;
            }
            pthread_create(&tid, NULL, worker_read, args);
        } else if (strcmp(args->cmd->action, "empty") == 0) {
            // ignore empty first
            pthread_create(&tid, NULL, worker_empty, args);
        }

        iter++;
    }

    return 0;
}
