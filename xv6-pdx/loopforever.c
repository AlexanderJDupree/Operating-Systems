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

int fork_process()
{
  int pid = fork();
  if(pid < 0) 
  {
    printf(2, "loopforever: Fork error occured. Exiting.");
    exit();
  }
  return pid;
}

void wait_for_child()
{
  if(wait() < 0)
  {
    printf(2, "loopforever: Wait error occured");
    exit();
  }
  return;
}

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

static void
loop()
{
  int i = 0; 
  while(i < ~0)
  {
    ++i;
  }
  exit();
}

int
main(int argc, char *argv[])
{
  if(argc < 2)
  {
    help();
    exit();
  }

  int wait = 0;
  int child_exit = 0;
  int children = atoi(argv[1]) - 1;

  if(argc >= 3 && strncmp(argv[2], "--wait", 6) == 0)
  {
    wait = 1;
  }
  if(argc >= 3 && strncmp(argv[2], "--child_exit", 12) == 0)
  {
    child_exit = 1;
  }

  for(int i = 0; i < children; ++i)
  {
    int pid = fork_process();
    if(pid == 0)
    {
      (child_exit) ? loop() : loopforever();
    }
  }
  printf(1, "\nCreated %d children.", children);
  if(wait)
  {
    printf(1, " Kill children to wakeup parent\n", children);
    while(children)
    {
      wait_for_child();
      --children;
    }
    printf(1, "\nParent Awake! Finished waiting for children!");
  }
  printf(1, " Starting loopforever on parent\n");
  loopforever();

  exit();
}