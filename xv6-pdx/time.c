/*
 * File         : time.c
 * Description  : User program to time process execution
 * Author       : Alexander DuPree
 * Date         : 13 Jan 2019
*/

#include "types.h"
#include "user.h"


// Wrapper that asserts fork succeeded
int fork_process();

// Wrapper that asserts wait succeeded
void wait_for_child();

int
main(int argc, char* argv[])
{
  int start_ticks = uptime();

  int pid = fork_process();

  if(pid == 0) // Child
  {
    exec(argv[1], argv + 1);
  }
  else // Parent
  {
    wait_for_child();

    int elapsed = uptime() - start_ticks;
    int seconds = elapsed / 1000;
    int ms      = elapsed % 1000;

    printf(1, "%s ran in %d.%ds\n", argv[1], seconds, ms);
  }
  exit();
}

int fork_process()
{
  int pid = fork();
  if(pid < 0) 
  {
    printf(2, "TIME: Fork error occured. Exiting.");
    exit();
  }
  return pid;
}

void wait_for_child()
{
  if(wait() < 0)
  {
    printf(2, "TIME: Wait error occured");
    exit();
  }
  return;
}