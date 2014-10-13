/***************************************************************************
 *  Title: MySimpleShell 
 * -------------------------------------------------------------------------
 *    Purpose: A simple shell implementation 
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: tsh.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: tsh.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __MYSS_IMPL__

/************System include***********************************************/
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/************Private include**********************************************/
#include "tsh.h"
#include "io.h"
#include "interpreter.h"
#include "runtime.h"

/************Student include***********************************************/
 #include <unistd.h>

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define BUFSIZE 80

/************Global Variables*********************************************/

/************Function Prototypes******************************************/
/* handles SIGINT and SIGSTOP signals */	
static void sig(int);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

int main (int argc, char *argv[])
{
  // printf("in main...\n");
  /* Initialize command buffer */
  char* cmdLine = malloc(sizeof(char*)*BUFSIZE);

  /* shell initialization */
  if (signal(SIGINT, sig) == SIG_ERR) PrintPError("SIGINT");
  if (signal(SIGTSTP, sig) == SIG_ERR) PrintPError("SIGTSTP");
  if (signal(SIGCHLD, sig) == SIG_ERR) PrintPError("SIGCHLD");

  while (!forceExit) /* repeat forever */
  {
    // printf("tsh> ");
    /* read command line */
    getCommandLine(&cmdLine, BUFSIZE);

    if((strcmp(cmdLine, "exit") == 0))
    {
      forceExit=TRUE;
      continue;
    }

    /* checks the status of background jobs */
    /* TODO in runtime.c */
    CheckJobs();

    /* interpret command and line
     * includes executing of commands */
    Interpret(cmdLine);
  }

  /* shell termination */
  free(cmdLine);
  return 0;
} /* end main */

static void sig(int signo)
{
  int wpid, status;
  // printf("pid:%d ppid: %d\n", pid, getppid());

  switch(signo){

    // kills the foreground process via signal to its process group
    case SIGINT:
      if (fg_pid >= 0)
      {
        kill(-fg_pid, SIGINT); // this removes from job list? 
      }
      break;
    
    case SIGTSTP:
      // printf("%d SIGTSTP signal in tsh\n", fg_pid);
      // if process is FG then send signal to its whole process group
      if (fg_pid >= 0)
      {
        kill(-fg_pid, SIGTSTP);
        fg_pid = -1;
        printf("finished sigtstp");
      }

      break;

    
    case SIGCHLD:
      do
      {
        wpid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (wpid == fg_pid)
        {
          fg_pid = -1;
        }
        if (WIFSTOPPED(status))
        {
          UpdateJobs(wpid, STOPPED);
        }
        else
        {
          UpdateJobs(wpid, DONE);
        }
        // if (WIFEXITED(status))
        // {
        //   // child ended normally
        //   // printf("in child end normally..\n");
        //   UpdateJobs(wpid, DONE);
        // }
        // else if (WIFSIGNALED(status))
        // {
        //   // child ended by another process, SIGINT = 2
        //   // printf("in child end by process..\n");
        //   if (WTERMSIG(status)==2)
        //   {
        //     UpdateJobs(wpid, DONE);
        //   }
        // }
        // else if (WIFSTOPPED(status))
        // {
        //   // printf("in child stop..\n");
        //   UpdateJobs(wpid, STOPPED);
        // } 
      } while(wpid>0);


      break;
    default:
      break;
  }  
}

