/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __RUNTIME_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

/************Private include**********************************************/
#include "runtime.h"
#include "io.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

typedef struct bgjob_l {
  pid_t pid;
  int jid;
  int state;
  char* cmdline;
  struct bgjob_l* next;
  bool print;
} bgjobL;

typedef struct alias_l {
  char* alias;
  char* cmd;
  struct alias_l* next;
} aliasL;

bgjobL *bgjobs = NULL;
aliasL *aliases = NULL;


int nextjid = 1;
int fg_pid = 0;

/************Function Prototypes******************************************/
/* run command */
static void RunCmdFork(commandT*, bool);
/* runs an external program command after some checks */
static void RunExternalCmd(commandT*, bool);
/* resolves the path and checks for exutable flag */
static bool ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void Exec(commandT*, bool);
/* runs a builtin command */
static void RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool IsBuiltIn(char*);
/* runs the cd command */
static void RunCd(commandT* cmd);
/* runs alias and unalias */
static void RunAlias(commandT* cmd);
static void RunUnAlias(commandT* cmd);
/* adds jobs to the job table */
static void AddJob(pid_t pid, int state, char* cmdline, bool print);
/* wait for process with pid to no longer be in the foreground */
static void WaitFg(pid_t jid);
/* find job with jid and return job */
static bgjobL* FindJobByJid(int jid);
/* find job with pid and return job */
static bgjobL* FindJobByPid(int pid);
/* free job memory */
static void ReleaseJob(bgjobL *job);
/* debug function to print jobs */
void PrintJobs();
void DebugPrintJobs();
/************External Declaration*****************************************/

/**************Implementation***********************************************/
int total_task;
void RunCmd(commandT** cmd, int n)
{
  int i;
  total_task = n;
  if(n == 1)
    RunCmdFork(cmd[0], TRUE);
  else{
    RunCmdPipe(cmd[0], cmd[1]);
    for(i = 0; i < n; i++)
      ReleaseCmdT(&cmd[i]);
  }
}

/* fork() system call - copies address space but stops parent execution */
void RunCmdFork(commandT* cmd, bool fork)
{
  if (cmd->argc<=0)
    return;
  if (IsBuiltIn(cmd->argv[0]))
  {
    RunBuiltInCmd(cmd);
  }
  else
  {
    RunExternalCmd(cmd, fork);
  }
}

void RunCmdBg(commandT* cmd)
{
  bgjobL* job;
  char* id = cmd->argv[1];
  char* command = cmd->argv[0];
  pid_t pid;

  // find job in list
  if (id == NULL)
  {
    printf("No id supplied\n");
    return;
  }
  else if (atoi(id))
  {
    pid = atoi(id);
    job = FindJobByJid(pid);
    if (job == NULL)
    {
      // printf("No job in job list pid\n");
      return;
    }
  }
  else
  {
    printf("Error in the command\n");
    return;
  }

  // send SIGCONT signal to the specified process group
  if (job->state==STOPPED || job->state==BACKGROUND)
  {
    kill(-(job->pid), SIGCONT);
    // if(ret < 0)
    // {
    //   printf("Error in kill\n");
    // }
  }

  // change the state of the jobs
  if (strcmp(command,"fg") == 0)
  {
    job->state = FOREGROUND;
    job->print = FALSE;
    fg_pid = job->pid;
    WaitFg(job->pid);
  }
  else
  {
    job->state = BACKGROUND;
  }
}

void RunCmdPipe(commandT* cmd1, commandT* cmd2)
{
  // TODO
}

void RunCmdRedirOut(commandT* cmd, char* file)
{
  // TODO
}

void RunCmdRedirIn(commandT* cmd, char* file)
{
  // TODO
}


/*Try to run an external command*/
static void RunExternalCmd(commandT* cmd, bool fork)
{
  if (ResolveExternalCmd(cmd)){
    Exec(cmd, fork);
  }
  else {
    printf("%s: command not found\n", cmd->argv[0]);
    fflush(stdout);
    ReleaseCmdT(&cmd);
  }
}

/*Find the executable based on search list provided by environment variable PATH*/
static bool ResolveExternalCmd(commandT* cmd)
{
  char *pathlist, *c;
  char buf[1024];
  int i, j;
  struct stat fs;

  if(strchr(cmd->argv[0],'/') != NULL){
    if(stat(cmd->argv[0], &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(cmd->argv[0],X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(cmd->argv[0]);
          return TRUE;
        }
    }
    return FALSE;
  }
  pathlist = getenv("PATH");
  if(pathlist == NULL) return FALSE;
  i = 0;
  while(i<strlen(pathlist)){
    c = strchr(&(pathlist[i]),':');
    if(c != NULL){
      for(j = 0; c != &(pathlist[i]); i++, j++)
        buf[j] = pathlist[i];
      i++;
    }
    else{
      for(j = 0; i < strlen(pathlist); i++, j++)
        buf[j] = pathlist[i];
    }
    buf[j] = '\0';
    strcat(buf, "/");
    strcat(buf,cmd->argv[0]);
    if(stat(buf, &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(buf,X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(buf); 
          return TRUE;
        }
    }
  }
  return FALSE; /*The command is not found or the user don't have enough priority to run.*/
}

static void Exec(commandT* cmd, bool forceFork)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTSTP);
  sigprocmask(SIG_BLOCK, &mask, NULL);

  pid_t pid;
  pid = fork();

  if (pid == 0)
  {
    // child
    setpgid(0,0);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    if (execv(cmd->name, cmd->argv) == -1)
    {
      perror("execv");
      exit(2);
    }
  }
  else
  {
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    if (cmd->bg == 1)
    {
      AddJob(pid, BACKGROUND, cmd->cmdline, TRUE);
    }
    else
    {
      fg_pid = pid;
      AddJob(pid, FOREGROUND, cmd->cmdline, FALSE);
      // waitpid(pid, &status, 0);
      WaitFg(pid);

    }
  }
}

static bool IsBuiltIn(char* cmd)
{
  if (strcmp(cmd, "bg") == 0 ||
      strcmp(cmd, "fg") == 0 ||
      strcmp(cmd, "jobs") == 0 ||
      strcmp(cmd, "cd") == 0 ||
      strcmp(cmd, "alias") == 0 ||
      strcmp(cmd, "unalias") == 0)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }    
}


static void RunBuiltInCmd(commandT* cmd)
{
  char* command = cmd->argv[0];
  // printf("%s\n",command);

  if (strcmp(command,"bg") == 0 || strcmp(command,"fg") == 0)
  {
    // printf("bg and fg command runs here\n");
    RunCmdBg(cmd);
  }
  else if (strcmp(command,"jobs") == 0)
  {
    PrintJobs();
  }
  else if (strcmp(command,"cd") == 0)
  {
    RunCd(cmd);
  }
  // else if (strcmp(command, "alias") == 0)
  // {
  //   RunAlias(cmd);
  // }
  // else if (strcmp(command, "unalias") == 0)
  // {
  //   RunUnAlias(cmd);
  // }
  else
  {
    printf("This should never happen. Whoops :( :(\n");
  }
}

static void RunCd(commandT* cmd)
{
  char* arg = cmd->argv[1];
  char* path = malloc(sizeof(size_t)*100);

  // move to home directory
  if (arg == NULL)
  {
    if (chdir(getenv("HOME")) != 0)
    {
      printf("error in cd\n");
    }
  }
  else
  {
    if (chdir(arg) != 0)
    {
      printf("error in cd\n");
    }
  }
  free(path);
}

static void RunAlias(commandT* cmd)
{

}

static void RunUnAlias(commandT* cmd)
{

}

static void WaitFg(pid_t pid)
{
  bgjobL* job = FindJobByPid(pid);
  while(job != NULL && job->state==FOREGROUND)
  {
    sleep(1);
  }
  return;
}

commandT* CreateCmdT(int n)
{
  int i;
  commandT * cd = malloc(sizeof(commandT) + sizeof(char *) * (n + 1));
  cd -> name = NULL;
  cd -> cmdline = NULL;
  cd -> is_redirect_in = cd -> is_redirect_out = 0;
  cd -> redirect_in = cd -> redirect_out = NULL;
  cd -> argc = n;
  for(i = 0; i <=n; i++)
    cd -> argv[i] = NULL;
  return cd;
}

/*Release and collect the space of a commandT struct*/
void ReleaseCmdT(commandT **cmd){
  int i;
  if((*cmd)->name != NULL) free((*cmd)->name);
  if((*cmd)->cmdline != NULL) free((*cmd)->cmdline);
  if((*cmd)->redirect_in != NULL) free((*cmd)->redirect_in);
  if((*cmd)->redirect_out != NULL) free((*cmd)->redirect_out);
  for(i = 0; i < (*cmd)->argc; i++)
    if((*cmd)->argv[i] != NULL) free((*cmd)->argv[i]);
  free(*cmd);
}

/*
    JOB TABLE FUNCTIONS:
      - AddJob
      - ReleaseJob

      - CheckJobs
      - UpdateJobs
      - PrintJobs

      - FindJobByJid
      - FindJobByPid
*/

/*                      ADDJOB

Constructs an entry for the background jobs list from a PID,
state, cmdline, and boolean indicating whether to print the job
when it's done. Foreground jobs are also added to the list but
given a JID of 0.

*/
void AddJob(pid_t pid, int state, char* cmdline, bool print)
{
  bgjobL* current = bgjobs;
  bgjobL* newJob = (bgjobL*) malloc(sizeof(bgjobL));
  newJob->pid = pid;
  newJob->jid = nextjid;
  newJob->state = state;
  newJob->cmdline = cmdline;
  newJob->next = NULL;
  newJob->print = print;

  if (state == BACKGROUND)
  {
    nextjid += 1; 
  }
  else
  {
    newJob->jid = 0; // give foreground jobs a jid of 0
  }

  if (current == NULL)
  {
    bgjobs = newJob;
  }
  else
  {
    while (current->next != NULL)
    {
      current = current->next;
    }
    current->next = newJob;
  }
}

void ReleaseJob(bgjobL* job)
{
  // free(job->state); this causes crashes, idk why
  free(job->cmdline);
  free(job);
}

void CheckJobs()
{
  bgjobL* current = bgjobs;
  bgjobL* prev = NULL;

  while (current != NULL)
  {
    if (current->state == DONE)
    {
      if (current->print)
      {
        printf("[%d]   Done                    %s\n", current->jid,current->cmdline);
      }
      else
      {
        fg_pid = -1;
      }

      if (prev == NULL)
      {
        bgjobs = current->next;
        ReleaseJob(current);
        current = bgjobs;
      }
      else
      {
        prev->next = current->next;
        ReleaseJob(current);
        current = prev->next;
      }

      fflush(stdout);
    }
    else
    {
      prev = current;
      current = current->next;
    }
  }
  if (bgjobs == NULL)
  {
    nextjid = 1;
  }
}

void UpdateJobs(pid_t pid, int state)
{
  bgjobL* job;
  job = FindJobByPid(pid);

  if (job != NULL)
  {
    job->state = state;
    if (state == STOPPED)
    {
      if (job->jid == 0)
      {
        job->jid = nextjid;
        nextjid += 1;
      }
      
      printf("[%d]   Stopped                 %s\n", job->jid, job->cmdline); 
      fflush(stdout);
    }
  }

}

void PrintJobs()
{
  bgjobL* current = bgjobs;
  while (current != NULL)
  {
    const char* state;
    if (current->state == BACKGROUND)
    {
      state = "Running";
    }
    else if (current->state == STOPPED)
    {
      state = "Stopped";
    }
    else
    {
      // There's a done or FG job in the list
      current = current->next;
      continue; // maybe
    }

    printf("[%d]   %s                 %s", current->jid, state, current->cmdline);

    if (current->state == BACKGROUND)
    {
      printf(" &\n");
    }
    else
    {
      printf("\n");
    }

    fflush(stdout);
    current = current->next;
  }
}

bgjobL* FindJobByJid(int jid)
{
  bgjobL* current = bgjobs;
  if (current == NULL)
  {
    return NULL;
  }
  else
  {
    do
    {
      if (current->jid == jid)
      {
        return current;
      }
      else
      {
        current = current->next;
      }
    } while(current != NULL);
    return NULL;
  }
}

bgjobL* FindJobByPid(pid_t pid)
{
  bgjobL* current = bgjobs;
  if (current == NULL)
  {
    return NULL;
  }
  else
  {
    do
    {
      if (current->pid == pid)
      {
        return current;
      }
      else
      {
        current = current->next;
      }
    } while(current != NULL);
    return NULL;
  }
}

/* */
void StopFgProc()
{
  if (fg_pid >= 0)
  {
    kill(-fg_pid, SIGTSTP);
    fg_pid = -1;
  }
}