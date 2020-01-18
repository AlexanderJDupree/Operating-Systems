#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "uproc.h"
#ifdef PDX_XV6
#include "pdx-kernel.h"
#endif // PDX_XV6

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  xticks = ticks;
  return xticks;
}

#ifdef PDX_XV6
// shutdown QEMU
int
sys_halt(void)
{
  do_shutdown();  // never returns
  return 0;
}
#endif // PDX_XV6


#ifdef CS333_P1

int
sys_date(void)
{
  struct rtcdate *date;

  // Retrieve user space rtcdate pointer from stack
  if(argptr(0, (void*)&date, sizeof(struct rtcdate)) < 0)
    return -1;

  // Mutates the rtcdate object in user space
  cmostime(date);
  
  return 0; // Success
}

#endif // CS333_P1

#ifdef CS333_P2

uint
sys_getuid(void)
{
  return myproc()->uid;
}

uint
sys_getgid(void)
{
  return myproc()->gid;
}

uint
sys_getppid(void)
{
  // Return parents PID if it exists, else this PID
  return myproc()->parent ? myproc()->parent->pid : sys_getpid();
}

int
sys_setuid(void)
{
  int uid;

  // Get argument off the stack
  if(argint(0, &uid) < 0 || !(UID_MIN <= uid && uid <= UID_MAX))
    return -1; // Failed to retrieve arg OR uid not in min/max range

  return myproc()->uid = uid; // Implicitly casts to uint
}

int
sys_setgid(void)
{
  int gid;

  // Get argument off the stack
  if(argint(0, &gid) < 0 || !(GID_MIN <= gid && gid <= GID_MAX))
    return -1; // Failed to retrieve arg OR gid not in min/max range

  return myproc()->gid = gid; // Implicitly casts to uint
}

int
sys_getprocs(void)
{
  int max;
  struct uproc* table;

  if(argint(0, &max) < 0 || max < 0)
    return -1;

  // Retrieve user space uproc pointer from stack
  if(argptr(1, (void*)&table, sizeof(struct uproc) * max) < 0)
    return -1;

  return getprocs(max, table);
}

#endif // CS333_P2