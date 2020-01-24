#ifndef XV6_UPROC_H
#define XV6_UPROC_H

#define STRMAX 32

struct uproc {
  uint pid;
  uint uid;
  uint gid;
  uint ppid;
#ifdef CS333_P4
  uint priority;
#endif // CS333_P4
  uint elapsed_ticks;
  uint cpu_ticks_total;
  char state[STRMAX];
  uint size;
  char name[STRMAX];
};

#endif // XV6_UPROC_H