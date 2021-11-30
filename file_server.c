#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CMD 0
#define PATH 1
#define INPUT 2
#define MAX_INP_SIZE 50
#define N_GLOCKS 2
#define READ_LOCK 0
#define EMPTY_LOCK 1

pthread_mutex_t glocks[N_GLOCKS];

struct worker_args {
    char cmd[MAX_INP_SIZE + 1];
    char path[MAX_INP_SIZE + 1];
    char input[MAX_INP_SIZE + 1];
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
    return;
    int prob = (rand() % 100);
    if (prob < 80) {
        sleep(1);
    } else {
        sleep(6);
    }
}

void rand_sleep(int min, int max) {
    return;
    int prob = (rand() % (max - min)) + min;
    sleep(prob);
}


/**
    COMMANDS LIB
**/
void *worker_write(void *_args) {
    struct worker_args *args = (struct worker_args *) _args;

    simulate_access();

    FILE *target_file;
    strcpy(args->path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    target_file = fopen(args->path, "a");

    if (target_file == NULL) {
        printf("[ERR] worker_write fopen\n");   
        exit(1);
    }

    fprintf(target_file,"%s", args->input);
    fclose(target_file);
    free(args);
}


void *worker_read(void *_args) {
    struct worker_args *args = (struct worker_args *) _args;

    simulate_access();

    FILE *from_file, *to_file;

    strcpy(args->path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    from_file = fopen(args->path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[READ_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd, args->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[READ_LOCK]);
    } else {
        pthread_mutex_lock(&glocks[READ_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary
        fprintf(to_file, "%s %s: ", args->cmd, args->path);
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);
        pthread_mutex_unlock(&glocks[READ_LOCK]);

        fclose(from_file);
    }

    free(args);
}


void *worker_empty(void *_args) {
    struct worker_args *args = (struct worker_args *) _args;

    simulate_access();

    FILE *from_file, *to_file;

    strcpy(args->path, "/home/derick/acad/cs140/proj2/main/program.txt"); // temporary
    from_file = fopen(args->path, "r");

    if (from_file == NULL) {
        pthread_mutex_lock(&glocks[EMPTY_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd, args->path);
        fclose(to_file);
        pthread_mutex_unlock(&glocks[EMPTY_LOCK]);
    } else {
        pthread_mutex_lock(&glocks[EMPTY_LOCK]);
        to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary
        int copy;
        if ((copy = fgetc(from_file)) == EOF) {
            fprintf(to_file, "%s %s: FILE DNE\n", args->cmd, args->path);
        } else {
            fprintf(to_file, "%s %s: ", args->cmd, args->path);
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
        from_file = fopen(args->path, "w");
        fclose(from_file);
    }

    rand_sleep(7, 10);
    free(args);
}



/**
    MAIN
**/
int main() {
    for(int i = 0; i < N_GLOCKS; i++)
        pthread_mutex_init(&glocks[i], NULL);

    char inp[] = "";
    while (1) {
        char **split = malloc(3 * sizeof(char *));
        for (int i = 0; i < 3; i++)
            split[i] = malloc((MAX_INP_SIZE + 1) * sizeof(char)); // free afterwards in thread


        get_input(inp, split);

        struct worker_args *args = (struct worker_args *)malloc(sizeof(worker_args));
        strcpy(args->cmd, split[CMD]);
        strcpy(args->path, split[PATH]);
        strcpy(args->input, split[INPUT]);

        for (int i = 0; i < 3; i++)
            free(split[i]);
        free(split);

        pthread_t tid;
        if (strcmp(args->cmd, "write") == 0) {
            pthread_create(&tid, NULL, worker_write, args);
        } else if (strcmp(args->cmd, "read") == 0) {
            pthread_create(&tid, NULL, worker_read, args);
        } else if (strcmp(args->cmd, "empty") == 0) {
            pthread_create(&tid, NULL, worker_empty, args);
        }
    }

    return 0;
}
