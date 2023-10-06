#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include "wsh.h"

pid_t child_pid = 0;
job *job_list = NULL;

int execute_with_pipe(char **args) {
    int pipefds[2]; // file descriptors for pipe
    int isBackground = 0; // flag to check if command should be executed in the background

    // Split the args into separate commands at each pipe
    char **cmd1 = args;
    char **cmd2 = NULL;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL; // terminate the first command here
            cmd2 = &args[i + 1]; // second command starts from the next element
            break;
        }
        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL; // remove the '&' and mark for background execution
            isBackground = 1;
        }
    }

    if (cmd2 == NULL) {
        fprintf(stderr, "wsh: pipe error\n");
        return -1;
    }

    if (pipe(pipefds) == -1) {
        perror("wsh");
        return -1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("wsh");
        return -1;
    }

    if (pid1 == 0) {
        // Child 1 - will write to the pipe
        close(pipefds[0]); // close reading end
        dup2(pipefds[1], STDOUT_FILENO); // make stdout as writing end
        close(pipefds[1]); // close the writing end descriptor after duplicating

        if (execvp(cmd1[0], cmd1) == -1) {
            perror("wsh");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("wsh");
        return -1;
    }

    if (pid2 == 0) {
        // Child 2 - will read from the pipe
        close(pipefds[1]); // close writing end
        dup2(pipefds[0], STDIN_FILENO); // make stdin as reading end
        close(pipefds[0]); // close the reading end descriptor after duplicating

        if (execvp(cmd2[0], cmd2) == -1) {
            perror("wsh");
            exit(EXIT_FAILURE);
        }
    }

    // Parent - close both ends of the pipe and wait for both children
    close(pipefds[0]);
    close(pipefds[1]);

    if (!isBackground) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    } else {
        // Handle background execution if necessary
        // You can add these processes to the job list or handle them as required
    }

    return 0;
}




void handle_sigint(int sig) {
    // ctrl+c handler
    if (child_pid > 0) {
        kill(child_pid, SIGINT);
        // printf("%d ctrl+c",child_pid);
        child_pid = 0;
    } else {
    }
}

void handle_sigtstp(int sig) {
    // ctrl+z handler
    if (child_pid > 0) {
        kill(child_pid, SIGSTOP);
        // printf("%d ctrl+z",child_pid);
        child_pid = 0;
    } else {
    }
}

job* get_job_by_id(int job_id) {
    for (job *j = job_list; j; j = j->next) {
        if (j->job_id == job_id) {
            return j;
        }
    }
    return NULL;
}

int get_max_job_id() {
    int max_id = 0;
    for (job *j = job_list; j; j = j->next) {
        if (j->job_id > max_id) {
            max_id = j->job_id;
        }
    }
    return max_id;
}

void add_job(job *j) {
    // Add the job to the job list
    j->next = job_list;
    job_list = j;
}

void remove_job(pid_t pgid) {
    // Remove the job from the job list
    job *j, *prev = NULL;
    for (j = job_list; j; j = j->next) {
        if (j->pgid == pgid) {
            if (prev) {
                prev->next = j->next;
            } else {
                job_list = j->next;
            }
            free(j);
            return;
        }
        prev = j;
    }
}

void print_jobs() {
    int maxJobId = get_max_job_id();

    // Print all jobs in ascending order of job ID
    for (int id = 1; id <= maxJobId; ++id) {
        job *j = get_job_by_id(id);
        if (j != NULL) {
            printf("%d: ", j->job_id);
            for (int i = 0; j->argv[i] != NULL && i < 128; i++) {
                printf("%s ", j->argv[i]);
            }
            printf("\n");
        }
    }
}



void executeCommand(char **args) {
    int background = 0;

    int pipe_found = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_found = 1;
            break;
        }
    }
    if(pipe_found){
        execute_with_pipe(args);
        return;
    }

    // Check if the last argument is "&" to run in background
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            background = 1;
            args[i] = NULL;  // Remove the '&'
            break;
        }
    }

    if (strcmp(args[0], "exit") == 0) {
        // Exit
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        // CD
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
    } else if (strcmp(args[0], "jobs") == 0) {
        // Jobs
        print_jobs();
    } else if (strcmp(args[0], "fg") == 0 || strcmp(args[0], "bg") == 0) {
        int job_id;
        if (args[1] == NULL) {
            // no args
            job_id = get_max_job_id();
            if (job_id == 0) {
                fprintf(stderr, "wsh: no current job\n");
                return;
            }
        } else {
            // 1 args
            job_id = atoi(args[1]);
        }

        job *j = get_job_by_id(job_id);
        if (j) {
            if (strcmp(args[0], "fg") == 0) {
            //     // FG
            //     tcsetpgrp(STDIN_FILENO, j->pgid);
            //     kill(-j->pgid, SIGCONT);
            //     int status;
            //     waitpid(-j->pgid, &status, WUNTRACED);
            //     if (WIFSTOPPED(status)) {
            //         j->notified = 1;
            //     } else {
            //         remove_job(j->pgid);
            //     }
            //     tcsetpgrp(STDIN_FILENO, getpgrp());
            // } else {
            //     // BG
            //     kill(-j->pgid, SIGCONT);
            //     printf("[%d] %d continued %s\n", j->job_id, j->pgid, j->command);
            }
        } else {
            fprintf(stderr, "wsh: job not found\n");
        }
    } else {
        // Execute other commands
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("wsh");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Fork error
            perror("wsh");
        } else {
            // Parent process
            if (!background) {
                child_pid = pid;
                int status;
                waitpid(pid, &status, WUNTRACED);  // foreground

                if (WIFSTOPPED(status)) {
                    // ctrl+z handler
                    job *j = malloc(sizeof(job));
                    j->command = strdup(args[0]);  // Save the command
                    int i = 0;

                    while (args[i] != NULL && i < 128) {
                        j->argv[i] = strdup(args[i]);  // Copy each arg
                        i++;
                    }
                    j->argv[i] = NULL;  // Null terminate the argv array

                    j->pgid = pid;  // Set the process group ID
                    j->job_id = get_max_job_id() + 1;  // Assign a new job ID
                    add_job(j);  // Add the job to the job list
                }
            }
            else {
                // background
                    job *j = malloc(sizeof(job));
                    j->command = strdup(args[0]);  // Save the command

                    int i = 0;
                    while (args[i] != NULL && i < 128) {
                        j->argv[i] = strdup(args[i]);  // Copy each arg
                        i++;
                    }
                    j->argv[i] = "&";  // background mark
                    j->argv[i+1] = NULL;

                    j->pgid = pid;  // Set the process group ID
                    j->job_id = get_max_job_id() + 1;  // Assign a new job ID
                    add_job(j);  // Add the job to the job list
            }
        }
    }
}


// shell
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

// echo helloworld | grep hello