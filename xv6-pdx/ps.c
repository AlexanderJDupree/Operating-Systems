/*
 * File         : ps.c
 * Description  : User program to display process information
 * Author       : Alexander DuPree
 * Date         : 13 Jan 2019
*/

#include "types.h"
#include "user.h"
#include "uproc.h"

static void
dumpfield(const char* field, int field_width)
{
  for(int i = 0; field[i] && field_width >= 0; ++i)
  {
    --field_width;
    putc(1, field[i]);
  }
  // Fill with spaces
  while(--field_width >= 0) { putc(1, ' '); }
}

static void
procdump(struct uproc* p)
{
  double elapsed = (p->elapsed_ticks) / 1000.0f;
  double cpu = p->cpu_ticks_total / 1000.0f;

  char buf[32]; // 32 digits can more than hold any 32 bit number

  dumpfield(itoa(p->pid, buf, 10), 8);
  dumpfield(p->name, 13);
  dumpfield(itoa(p->uid, buf, 10), 11);
  dumpfield(itoa(p->gid, buf, 10), 8);
  dumpfield(itoa(p->ppid, buf, 10), 8);
  dumpfield(dtoa(elapsed, buf, 3), 10);
  dumpfield(dtoa(cpu, buf, 3), 6);
  dumpfield(p->state, 8);
  dumpfield(itoa(p->size, buf, 10), 8);
  printf(1, "\n");

  return;
}

static void
ps(uint max)
{
#define HEADER "\nPID\tName         UID\tGID\tPPID\tElapsed\t  CPU\tState\tSize\n"

  typedef struct uproc uproc;

  uproc* table;

  if((table = (uproc*) malloc(sizeof(uproc) * max)) == NULL)
  {
    printf(2, "PS: Failed to allocate process table\n");
    return;
  }

  // Check malloc didn't return null and getrprocs was sucessful
  if(!table || (max = getprocs(max, table)) < 0)
  {
    printf(2, "PS: Failed to retrieve process information\n");
  }
  else
  {
    printf(1, HEADER);
    for(struct uproc* p = table; p < table + max; ++p)
    {
      procdump(p);
    }
  }
  free(table);
}

static void
help()
{
  printf(2, "\nUsage: ps [OPTIONS]\n\n  -m <num>\tnumber of processes to display\n");
}

int
main(int argc, char* argv[])
{
  int max = 64;

  if(argc < 2 || (strncmp(argv[1], "-m", 2) == 0 && (max = atoi(argv[2])) > 0))
  {
    ps(max);
  }
  else
  {
    printf(2, "PS: Failed to parse arguments\n");
    help();
  }
  exit();
}
