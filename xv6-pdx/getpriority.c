/*
 * File         : getpriority.c
 * Description  : User program to get priority of a process
 * Author       : Alexander DuPree
 * Date         : 5 Mar 2019
*/

#include "types.h"
#include "user.h"
#include "pdx.h"

static void
help()
{
  printf(2 ,"\nUsage: getpriority <pid>, get priority of process\n\n");
}

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    help();
    exit();
  }

  int pid = atoi(argv[1]);
  int rc = getpriority(pid);

  if(rc >= 0)
  {
    printf(1, "\nPID: %d, PRIORITY: %d\n", pid, rc);
  }
  else
  {
    printf(2, "\ngetpriority: ERROR, failed to get priority\n");
  }
  exit();
}