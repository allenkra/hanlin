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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int execute_with_pipe(char **args) {
    int pipefd[2];
    int prev_pipe_read_end = -1;
    pid_t pid;
    char **current_command = args;

    while (*current_command) {
        char **next_command = current_command;
        int pipe_found = 0;

        // Locate the next pipe or the end of the commands
        while (*next_command && !pipe_found) {
            if (strcmp(*next_command, "|") == 0) {
                *next_command = NULL;
                pipe_found = 1;
            } else {
                next_command++;
            }
        }

        if (pipe(pipefd) == -1) {
            perror("wsh");
            return 1;
        }

        if ((pid = fork()) == 0) {
            // Child process
            close(pipefd[0]);
            if (pipe_found) dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            if (prev_pipe_read_end != -1) {
                dup2(prev_pipe_read_end, STDIN_FILENO);
                close(prev_pipe_read_end);
            }

            if (execvp(*current_command, current_command) == -1) {
                perror("wsh");
                exit(EXIT_FAILURE);
            }
        }

        close(pipefd[1]);
        if (prev_pipe_read_end != -1) close(prev_pipe_read_end);
        if (!pipe_found) break;

        prev_pipe_read_end = pipefd[0];
        current_command = next_command + 1;
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0);

    return 0; // Assuming 0 for success
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