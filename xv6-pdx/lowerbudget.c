/*
 * File         : lowerbudget.c
 * Description  : User program to lower the current budget of a process
 * Author       : Alexander DuPree
 * Date         : 5 Mar 2019
*/

#include "types.h"
#include "user.h"
#include "pdx.h"

static void
help()
{
  printf(2 ,"\nUsage: lowerbudget <pid> <diff>, lower budget of a process\n\n");
}

int main(int argc, char* argv[])
{
  if(argc != 3)
  {
    help();
    exit();
  }

  int pid = atoi(argv[1]);
  int diff = atoi(argv[2]);

  if(lowerbudget(pid, diff) < 0)
  {
    printf(2, "\nlowerbudget: ERROR, failed to lower budget\n");
  }
  exit();
}