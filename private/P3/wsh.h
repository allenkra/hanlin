#ifndef WSH_H
#define WSH_H

#include <stdio.h>
#include <stdlib.h>

void executeCommand(char **args);

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
} job;



// global var
pid_t child_pid;

#endif // WSH_H
