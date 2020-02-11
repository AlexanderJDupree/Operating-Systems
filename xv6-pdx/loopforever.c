/*
 * File         : loopforever.c
 * Description  : User program to create N processes that will loop forever.
 *                Important, this program is designed to be run in the background
 * Author       : Alexander DuPree
 * Date         : 10 Feb 2019
*/

#include "types.h"
#include "user.h"
#include "pdx.h"

static void
help()
{
  printf(2, "\nUsage: loopforever <num>, number of processes to run in the background\n\n");
}

static void
loopforever()
{
  int i = 0; 
  while(1)
  {
    ++i;
  }
}

int
main(int argc, char *argv[])
{
  if(argc != 2)
  {
    help();
    exit();
  }

  int children = atoi(argv[1]) - 1;


  for(int i = 0; i < children; ++i)
  {
    int pid = fork();
    if(pid == -1)
    {
      printf(2, "\nloopforever: ERROR, failed to create process");
      exit();
    }
    if(pid == 0)
    {
      loopforever();
    }
  }
  printf(1, "\nCreated %d children, starting loopforever on parent\n", children);
  loopforever();

  exit(); // Never gets called
}