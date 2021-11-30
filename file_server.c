#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define TYPE 0
#define PATH 1
#define STR 2


struct worker_args {
    char **cmd;
}worker_args;

/**
    HELPER LIB
**/
void get_input(char *inp, char *cmd[]) {
    scanf("%[^\n]%*c", inp);

    int i = 0;
    char *ptr = strtok(inp, " ");
    while (ptr != NULL) {
        cmd[i++] = ptr;
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
    simulate_access();

    FILE *target_file;

    struct worker_args *args = (struct worker_args *) _args;
    (args->cmd)[PATH] = "/home/derick/acad/cs140/proj2/main/program.txt"; // temporary
    target_file = fopen(args->cmd[PATH], "a");

    if (target_file == NULL) {
        printf("[ERR] worker_write fopen\n");   
        exit(1);
    }

    fprintf(target_file,"%s", args->cmd[STR]);
    fclose(target_file);
}


void *worker_read(void *_args) {
    simulate_access();

    FILE *from_file, *to_file;
    struct worker_args *args = (struct worker_args *) _args;
    args->cmd[PATH] = "/home/derick/acad/cs140/proj2/main/program.txt"; // temporary
    from_file = fopen(args->cmd[PATH], "r");
    to_file = fopen("/home/derick/acad/cs140/proj2/main/read.txt", "a"); // temporary

    if (from_file == NULL) {
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd[TYPE], args->cmd[PATH]);
    } else {
        fprintf(to_file, "%s %s: ", args->cmd[TYPE], args->cmd[PATH]);
        int copy;
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);

        fclose(from_file);
    }

    fclose(to_file);
}


void *worker_empty(void *_args) {
    simulate_access();

    FILE *from_file, *to_file;
    struct worker_args *args = (struct worker_args *) _args;
    args->cmd[PATH] = "/home/derick/acad/cs140/proj2/main/program.txt"; // temporary
    from_file = fopen(args->cmd[PATH], "r");
    to_file = fopen("/home/derick/acad/cs140/proj2/main/empty.txt", "a"); // temporary

    if (from_file == NULL) {
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd[TYPE], args->cmd[PATH]);
        fclose(to_file);
        return 0;
    }

    int copy;
    if ((copy = fgetc(from_file)) == EOF) {
        fprintf(to_file, "%s %s: FILE DNE\n", args->cmd[TYPE], args->cmd[PATH]);
    } else {
        fprintf(to_file, "%s %s: ", args->cmd[TYPE], args->cmd[PATH]);
        fputc(copy, to_file); // sleep per character???
        while ((copy = fgetc(from_file)) != EOF)
            fputc(copy, to_file);

        // newline
        fputc(10, to_file);
    }
    fclose(from_file);

    // empty
    from_file = fopen(args->cmd[PATH], "w");
    fclose(from_file);

    fclose(to_file);

    rand_sleep(7, 10);
}



/**
    MAIN
**/
int MAX_INP_SIZE = 50;
int main() {
    char inp[] = "";
    while (1) {
        char **cmd = malloc(3 * sizeof(char *));
        for (int i = 0; i < 3; i++) {
            cmd[i] = malloc((MAX_INP_SIZE + 1) * sizeof(char)); // free afterwards in thread
        }

        get_input(inp, cmd);

        struct worker_args *args = malloc(sizeof(struct worker_args));
        args->cmd = cmd;

        pthread_t *tid = malloc(sizeof(tid));
        if (strcmp(cmd[TYPE], "write") == 0) {
            pthread_create(tid, NULL, worker_write, (void*)args);
        } else if (strcmp(cmd[TYPE], "read") == 0) {
            pthread_create(tid, NULL, worker_read, (void*)args);
        } else if (strcmp(cmd[TYPE], "empty") == 0) {
            pthread_create(tid, NULL, worker_empty, (void*)args);
        }
    }

    return 0;
}
