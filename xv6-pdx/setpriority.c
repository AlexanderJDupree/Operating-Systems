/*
 * File         : setpriority.c
 * Description  : User program to setpriority of a process
 * Author       : Alexander DuPree
 * Date         : 5 Mar 2019
*/

#include "types.h"
#include "user.h"
#include "pdx.h"

static void
help()
{
  printf(2 ,"\nUsage: setpriority <pid> <priority>, set priority of process\n\n");
}

int main(int argc, char* argv[])
{
  if(argc != 3)
  {
    help();
    exit();
  }

  int pid = atoi(argv[1]);
  int priority = atoi(argv[2]);

  if(setpriority(pid, priority) >= 0)
  {
    printf(1, "\nPID: %d, PRIORITY: %d\n", pid, priority);
  }
  else
  {
    printf(2, "\nsetpriority: ERROR, failed to set new priority\n");
  }
  exit();
}