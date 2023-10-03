#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void executeCommand(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        // exit
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        // cd
        if (args[1] == NULL) {
            fprintf(stderr, "wsh: expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("wsh");
            }
        }
    } else {
        // execute
        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(args[0], args) == -1) { 
                perror("wsh");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("wsh");
        } else {
            wait(NULL);
        }
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    FILE *input = stdin;

    if (argc > 2) {
        // wrong beginning
        fprintf(stderr, "wsh: too many arguments\n");
        return 1;
    } else if (argc == 2) {
        // open file to get command
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("wsh");
            return 1;
        }
    }

    while (1) {
        if (input == stdin) {
            printf("wsh> ");
        }

        if (getline(&line, &len, input) == -1) {
            // bad input
            break;
        }

        char *args[128];
        char *token = strtok(line, " \t\n");
        int i = 0;

        while (token != NULL) {
            // get all args
            args[i++] = token;
            token = strtok(NULL, " \t\n"); // token get every element seperated by space and \t \n
        }
        // mark the end
        args[i] = NULL;

        if (i > 0) {
            executeCommand(args);
        }
    }

    free(line);
    
    if (input != stdin) {
        fclose(input);
    }

    return 0;
}
