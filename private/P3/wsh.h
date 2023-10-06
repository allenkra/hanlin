#ifndef WSH_H
#define WSH_H

#include <stdio.h>
#include <stdlib.h>

typedef struct process
{
  struct process *next;       /* next process in pipeline */
  char **argv;                /* for exec */
  pid_t pid;                  /* process ID */
  char completed;             /* true if process has completed */
  char stopped;               /* true if process has stopped */
  int status;                 /* reported status value */
} process;

/* A job is a pipeline of processes.  */
typedef struct job
{
  struct job *next;           /* next active job */
  char *command;              /* command line, used for messages */
  process *first_process;     /* list of processes in this job */
  pid_t pgid;                 /* process group ID */
  char notified;              /* true if user told about stopped job */
  int stdin, stdout, stderr;  /* standard i/o channels */
  int job_id;
} job;



// global var
pid_t child_pid;
job *job_list;



void executeCommand(char **args);
void add_job(job *j);
void remove_job(pid_t pgid);
void print_jobs();
void handle_sigint(int sig);
job* get_job_by_id(int job_id);
int get_max_job_id();
#endif // WSH_H
