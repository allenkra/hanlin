#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
pid_t child_pid = 0;

void handle_sigint(int sig) {
    if (child_pid > 0) {
        kill(child_pid, SIGINT);
        printf("%d ctrl+c",child_pid);
        child_pid = 0;
    } else {
        fflush(stdout);
    }
}

void handle_sigtstp(int sig) {
    if (child_pid > 0) {
        kill(child_pid, SIGSTOP);
        printf("%d ctrl+z",child_pid);
        child_pid = 0;
    } else {
        fflush(stdout);
    }
}



void executeCommand(char **args) {
    int background = 0;

    // check if the last argument is "&" to run in background
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            background = 1;
            args[i] = NULL;  // remove the '&'
            break;
        }
    }

    if (strcmp(args[0], "exit") == 0) {
        // exit
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        // cd
        if (args[1] == NULL) {
            fprintf(stderr, "wsh: expected argument to \"cd\"\n");
        } else if (args[2] != NULL) {
            fprintf(stderr, "wsh: too many arguments to \"cd\"\n");
        }
        else {
            if (chdir(args[1]) != 0) {
                perror("wsh");
            }
        }
    } else {
        // execute
        pid_t pid = fork();
        if (pid == 0) {
            // child process
            if (execvp(args[0], args) == -1) {
                perror("wsh");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // fork error
            perror("wsh");
        } else {
            // shell wait for child
            // background not wait
            if (!background) {
                child_pid = pid;
                waitpid(pid, NULL, 0);  // foreground
            }
        }
    }
}


int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);  // handle CTRL+C
    signal(SIGTSTP, handle_sigtstp);

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
