#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "uproc.h"
#include "spinlock.h"

static char *states[] = {
[UNUSED]    "unused",
[EMBRYO]    "embryo",
[SLEEPING]  "sleep ",
[RUNNABLE]  "runble",
[RUNNING]   "run   ",
[ZOMBIE]    "zombie"
};

#ifdef CS333_P3

#define statecount NELEM(states)
// record with head and tail pointer for constant-time access to the beginning
// and end of a linked list of struct procs.  use with stateListAdd() and
// stateListRemove().
struct ptrs {
  struct proc* head;
  struct proc* tail;
};
#endif // CS333_P3

static struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3
  struct ptrs list[statecount];
#endif // CS333_P3
#ifdef CS333_P4
  struct ptrs ready[MAXPRIO + 1];
  uint promoteAtTime;
#endif // CS333_P4
} ptable;

#ifdef CS333_P3
// list management function prototypes
static void initProcessLists(void);
static void initFreeList(void);
static void stateListAdd(struct ptrs*, struct proc*);
static int  stateListRemove(struct ptrs*, struct proc* p);
static void assertState(struct proc*, enum procstate, const char *, int);


// Macros record the function and line for the calling functions
#define transition(A, B, p) __transition(A, B, p, __FUNCTION__, __LINE__)
#define atom_transition(A, B, p) __atom_transition(A, B, p, __FUNCTION__, __LINE__)

// transition assumes the lock is already held when it manipulates the state lists
static void __transition(enum procstate, enum procstate, struct proc* p, const char*, int);
// atom_transition will acquire ptable lock then call transition
static void __atom_transition(enum procstate, enum procstate, struct proc* p, const char*, int);

// Higher-Order Function prototypes
static void* foldr(void* (*f)(struct proc*, void*), void* acc, struct proc* list);
static void* foldl(void* (*f)(void*, struct proc*), void* acc, struct proc* list);
static void  map(void (*f)(struct proc*), struct proc* list);
static struct proc* find_if(int (*f)(struct proc*));
#endif // CS333_P3

#ifdef CS333_P4

static void promote_all_procs(uint levels);
static void adjust_priority(struct proc* p, int diff);

#endif // CS333_P4

static struct proc *initproc;

uint nextpid = 1;
extern void forkret(void);
extern void trapret(void);
static void wakeup1(void* chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid) {
      return &cpus[i];
    }
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
#ifdef CS333_P3
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  if((p = ptable.list[UNUSED].head) == NULL) { // No unused processes
    release(&ptable.lock); 
    return 0;
  }

  transition(UNUSED, EMBRYO, p);

  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    atom_transition(EMBRYO, UNUSED, p); // Recover from failure
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p->start_ticks     = ticks;
  p->cpu_ticks_in    = 0;
  p->cpu_ticks_total = 0;

#ifdef CS333_P4
  p->priority = MAXPRIO;
  p->budget   = DEFAULT_BUDGET;
#endif //CS333_P4

  return p;
}
#else
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  int found = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED) {
      found = 1;
      break;
    }
  if (!found) {
    release(&ptable.lock);
    return 0; // return NULL
  }
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

#ifdef CS333_P1
  p->start_ticks     = ticks;
#endif // CS333_P1
#ifdef CS333_P2
  p->cpu_ticks_in    = 0;
  p->cpu_ticks_total = 0;
#endif // CS333_P2
  return p;
}
#endif // CS333_P3

//PAGEBREAK: 32
// Set up first user process.
#ifdef CS333_P3
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  // Initialize State Lists, add all processes to the UNUSED list
  acquire(&ptable.lock);
  initProcessLists();
  initFreeList();
  release(&ptable.lock);

#ifdef CS333_P4
  ptable.promoteAtTime = ticks + TICKS_TO_PROMOTE;
#endif //CS333_P4

  if((p = allocproc()) == NULL)
    panic("userinit: Failed to allocate init process");

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->uid = ROOT_UID;
  p->gid = ROOT_GID;

  atom_transition(EMBRYO, RUNNABLE, p);
}
#else
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

#ifdef CS333_P2
  p->uid = ROOT_UID;
  p->gid = ROOT_GID;
#endif // CS333_P2

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);
  p->state = RUNNABLE;
  release(&ptable.lock);
}
#endif // CS333_P3

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
#ifdef CS333_P3
int
fork(void)
{
  int i;
  uint pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    // Page alloc failed, transition back to unused
    atom_transition(EMBRYO, UNUSED, np);
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  np->uid = curproc->uid;
  np->gid = curproc->gid;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  atom_transition(EMBRYO, RUNNABLE, np);

  return pid;
}
#else
int
fork(void)
{
  int i;
  uint pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

#ifdef CS333_P2
  np->uid = curproc->uid;
  np->gid = curproc->gid;
#endif // CS333_P2

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}
#endif // CS333_P3

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.

#ifdef CS333_P3
void
exit(void)
{
  int fd;
  struct proc *curproc = myproc();

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init, no need to check UNUSED list
  for(enum procstate state = EMBRYO; state <= ZOMBIE; ++state)
  {
    map( LAMBDA(void _(struct proc* p){
      if(p->parent == curproc){
        p->parent = initproc;  // Adopt orphans
        if(p->state == ZOMBIE) // Reap child zombies
          wakeup1(initproc);
      }
    }), ptable.list[state].head);
  }
#ifdef CS333_P4
  // Search ready list
  for(int i = 0; i <= MAXPRIO; ++i)
  {
    map( LAMBDA(void _(struct proc* p){
      if(p->parent == curproc){
        p->parent = initproc;
      }
    }), ptable.ready[i].head);
  }
#endif // CS333_P4

  transition(RUNNING, ZOMBIE, curproc);

  curproc->sz = 0;

  sched(); // Jump into scheduler, never to return

  panic("zombie exit");
}
#else
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE) // Reap children
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
#ifdef PDX_XV6
  curproc->sz = 0;
#endif // PDX_XV6
  sched();
  panic("zombie exit");
}
#endif // CS333_P3

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.

#ifdef CS333_P3
int
wait(void)
{
  uint pid = 0;
  int havekids = 0;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){

    struct proc* child = find_if( LAMBDA(int _(struct proc* p){
      if(p->parent == curproc)
      {
        havekids = 1;
        return p->state == ZOMBIE;
      }
      return 0;
    }));

    if(child != NULL) // Reap child
    {
      pid = child->pid;
      kfree(child->kstack);
      child->kstack = 0;
      freevm(child->pgdir);
      child->pid = 0;
      child->parent = 0;
      child->name[0] = 0;
      child->killed = 0;

      transition(ZOMBIE, UNUSED, child);
      release(&ptable.lock);
      return pid;
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif // CS333_P3

#if defined(CS333_P4)
//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process

    acquire(&ptable.lock);

    if(ticks >= ptable.promoteAtTime)
    {
      promote_all_procs(1);
      ptable.promoteAtTime = ticks + TICKS_TO_PROMOTE;
    }

    // Find Highest Priority process
    for(int i = MAXPRIO; i >= 0 && !(p = ptable.ready[i].head); --i);

    if(p != NULL) {

      idle = 0;  // not idle this timeslice

      c->proc = p;
      switchuvm(p);

      transition(RUNNABLE, RUNNING, p);

      p->cpu_ticks_in = ticks;
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#elif defined(CS333_P3)
// CS333_P3 assumes PDX_XV6 is also defined. No need to clutter
// the code with extra ifdef PDX_XV6 checks
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process

    acquire(&ptable.lock);
    if((p = ptable.list[RUNNABLE].head) != NULL) {

      idle = 0;  // not idle this timeslice

      c->proc = p;
      switchuvm(p);

      transition(RUNNABLE, RUNNING, p);

      p->cpu_ticks_in = ticks;
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#else
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
#ifdef PDX_XV6
      idle = 0;  // not idle this timeslice
#endif // PDX_XV6
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
#ifdef CS333_P2
      p->cpu_ticks_in = ticks;
#endif // CS333_P2
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
  }
}
#endif // CS333_P3

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");

#ifdef CS333_P2
  int elapsed = ticks - p->cpu_ticks_in;
  p->cpu_ticks_total += elapsed;
#endif // CS333_P2
#ifdef CS333_P4
  // Prevent integer underflow
  p->budget -= (p->budget > 0) ? elapsed : 0;

  if(p->budget <= 0 && p->priority > 0)
    adjust_priority(p, -1);
#endif // CS333_P4

  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
#ifdef CS333_P3
void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock
  transition(RUNNING, RUNNABLE, curproc);
  sched();
  release(&ptable.lock);
}
#else
void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock
  curproc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}
#endif // CS333_P3

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
#ifdef CS333_P3
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk != &ptable.lock){  
    acquire(&ptable.lock); 
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;

  transition(RUNNING, SLEEPING, p);

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#else
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#endif // CS333_P3

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
#ifdef CS333_P3
static void
wakeup1(void *chan)
{
  // We already have the lock, do not acquire again
  map( LAMBDA(void _(struct proc* p){
    if(p->chan == chan){
      transition(SLEEPING, RUNNABLE, p);
    }
  } ), ptable.list[SLEEPING].head);
}
#else
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)

      p->state = RUNNABLE;
}
#endif // CS333_P3

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).

#ifdef CS333_P3
int
kill(int pid)
{
  acquire(&ptable.lock);
  struct proc* process = find_if( LAMBDA(int _(struct proc* p){
    return (p->pid == pid) ? 1 : 0;
  }));

  if(process != NULL)
  {
    process->killed = 1;

    // Wake process from sleep if necessary.
    if(process->state == SLEEPING)
      transition(SLEEPING, RUNNABLE, process);

    release(&ptable.lock);
    return 0;
  }
  release(&ptable.lock);
  return -1;
}
#else
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#endif // CS333_P3

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.

#if defined(CS333_P2)

static void
dumpfield(const char* field, int field_width)
{
  for(int i = 0; field[i] && field_width >= 0; ++i)
  {
    --field_width;
    cputc(field[i]);
  }
  // Fill with spaces
  while(--field_width >= 0) { cputc(' '); }
}

void
procdumpP2(struct proc* p, const char* state)
{
  // If parent is NULL - Set ppid to pid
  int ppid = (p->parent) ? p->parent->pid : p->pid;

  double cpu = p->cpu_ticks_total / 1000.0f;
  double elapsed = (ticks - p->start_ticks) / 1000.0f;

  char buf[32]; // 32 digits can more than hold any 32 bit number

  dumpfield(itoa(p->pid, buf, 10), 8);
  dumpfield(p->name, 13);
  dumpfield(itoa(p->uid, buf, 10), 11);
  dumpfield(itoa(p->gid, buf, 10), 8);
  dumpfield(itoa(ppid, buf, 10), 8);
  dumpfield(dtoa(elapsed, buf, 3), 10);
  dumpfield(dtoa(cpu, buf, 3), 6);
  dumpfield(state, 8);
  dumpfield(itoa(p->sz, buf, 10), 8);

  return;
}
#endif // CS333_P2

#if defined(CS333_P3)

static uint
length(struct ptrs list)
{
  uint length = 0;
  acquire(&ptable.lock);

  foldr( LAMBDA(void* _(struct proc* p, void* sum){
    *((uint*) sum) += 1;
    return sum;
  }), &length, list.head);

  release(&ptable.lock);
  return length;
}

static void
dumpList(struct ptrs list, void (*info)(struct proc*))
{
  acquire(&ptable.lock);
  map(info, list.head); // Apply info to each node in the list
  release(&ptable.lock);

  cputc('\n');
}

static void
dumpReadyList()
{
#ifdef CS333_P4
  for(int i = MAXPRIO; i >= 0; --i)
  {
    cprintf("\nPriority %d: ", i);
    dumpList(ptable.ready[i], LAMBDA(void _(struct proc* p){
      (p->next) ? cprintf("(%d, %d) -> ", p->pid, p->budget) 
                : cprintf("(%d, %d)", p->pid, p->budget);
    }));
  }
#else
  dumpList(ptable.list[RUNNABLE], LAMBDA(void _(struct proc* p){
      (p->next) ? cprintf("(%d) -> ", p->pid) : cprintf("(%d)", p->pid);
  }));
#endif // CS333_P4
}

static void
dumpSleepList()
{
  dumpList(ptable.list[SLEEPING], LAMBDA(void _(struct proc* p){
      (p->next) ? cprintf("(%d) -> ", p->pid) : cprintf("(%d)", p->pid);
  }));
}

static void
dumpZombieList()
{
  dumpList(ptable.list[ZOMBIE], LAMBDA(void _(struct proc* p){
      uint ppid = (p->parent) ? p->parent->pid : p->pid;

      (p->next) ? cprintf("(%d, %d) -> ", p->pid, ppid) : cprintf("(%d, %d)", p->pid, ppid);
  }));
}

static void
printListStats()
{
  int i, count, total = 0;
  struct proc *p;

  acquire(&ptable.lock);
  for (i=UNUSED; i<=ZOMBIE; i++) {
    count = 0;
    p = ptable.list[i].head;
    while (p != NULL) {
      count++;
      p = p->next;
    }
    cprintf("\n%s list has ", states[i]);
    if (count < 10) cprintf(" ");  // line up columns
    cprintf("%d processes", count);
    total += count;
  }
  release(&ptable.lock);
  cprintf("\nTotal on lists is: %d. NPROC = %d. %s",
      total, NPROC, (total == NPROC) ? "Congratulations!\n" : "Bummer\n");
  return;
}

void
statelistdump(int state)
{
  switch(state)
  {
    case RUNNABLE : 
      cprintf("\nReady List Processes:\n");
      dumpReadyList();
      break;
    case SLEEPING : 
      cprintf("\nSleep List Processes:\n");
      dumpSleepList();
      break;
    case ZOMBIE :
      cprintf("\nZombie List Processes:\n");
      dumpZombieList();
      break;
    case UNUSED : 
      cprintf("\nFree List Size: %d\n", length(ptable.list[UNUSED]));
      break;
    case -2 :
      printListStats();
      break;
    default:
      procdump();
  }
}

void
procdumpP3(struct proc* p, const char* state)
{
  // Resuse Project 2 Proc dump
  procdumpP2(p, state);
}

#endif //CS333_P3

#if defined(CS333_P4)
// TODO for Project 4, define procdumpP4() here
void
procdumpP4(struct proc* p, const char* state)
{
  // If parent is NULL - Set ppid to pid
  int ppid = (p->parent) ? p->parent->pid : p->pid;

  double cpu = p->cpu_ticks_total / 1000.0f;
  double elapsed = (ticks - p->start_ticks) / 1000.0f;

  char buf[32]; // 32 digits can more than hold any 32 bit number

  dumpfield(itoa(p->pid, buf, 10), 8);
  dumpfield(p->name, 13);
  dumpfield(itoa(p->uid, buf, 10), 11);
  dumpfield(itoa(p->gid, buf, 10), 8);
  dumpfield(itoa(ppid, buf, 10), 8);
  dumpfield(itoa(p->priority, buf, 10), 8);
  dumpfield(dtoa(elapsed, buf, 3), 10);
  dumpfield(dtoa(cpu, buf, 3), 6);
  dumpfield(state, 8);
  dumpfield(itoa(p->sz, buf, 10), 8);

  return;
}

#elif defined(CS333_P1)
// TODO for Project 1, define procdumpP1() here
void
procdumpP1(struct proc* p, const char* state)
{
  int elapsed = ticks - p->start_ticks;
  int seconds = elapsed / 1000;
  int ms      = elapsed % 1000;

  cprintf("%d\t%s\t     %d.%ds\t%s\t%d\t", p->pid, p->name, seconds, ms, state, p->sz);
  return;
}
#endif


void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

#if defined(CS333_P4)
#define HEADER "\nPID\tName         UID\tGID\tPPID\tPrio\tElapsed\t  CPU\tState\tSize\t PCs\n"
#elif defined(CS333_P3)
#define HEADER "\nPID\tName         UID\tGID\tPPID\tElapsed\t  CPU\tState\tSize\t PCs\n"
#elif defined(CS333_P2)
#define HEADER "\nPID\tName         UID\tGID\tPPID\tElapsed\t  CPU\tState\tSize\t PCs\n"
#elif defined(CS333_P1)
#define HEADER "\nPID\tName         Elapsed\tState\tSize\t PCs\n"
#else
#define HEADER "\n"
#endif

  cprintf(HEADER);  // not conditionally compiled as must work in all project states

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#if defined(CS333_P4)
    procdumpP4(p, state);
#elif defined(CS333_P3)
    procdumpP3(p, state);
#elif defined(CS333_P2)
    procdumpP2(p, state);
#elif defined(CS333_P1)
    procdumpP1(p, state);
#else
    cprintf("%d\t%s\t%s\t", p->pid, p->name, state);
#endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

#ifdef CS333_P2
int
getprocs(uint max, struct uproc* utable)
{
  int i = 0;
  acquire(&ptable.lock);
  struct proc *p;

  for(p = ptable.proc; p < ptable.proc + NPROC && i < max; p++)
  {
    if(p->state != UNUSED && p->state != EMBRYO)
    {
      struct uproc* uproc = utable + i++;

      uproc->pid  = p->pid;
      uproc->uid  = p->uid;
      uproc->gid  = p->gid;
      uproc->size = p->sz;

      // If parent exists, set ppid to their pid, else process pid
      uproc->ppid = (p->parent) ? p->parent->pid : p->pid;

      uproc->elapsed_ticks   = ticks - p->start_ticks;
      uproc->cpu_ticks_total = p->cpu_ticks_total;

#ifdef CS333_P4
      uproc->priority = p->priority;
#endif // CS333_P4

      safestrcpy(uproc->name, p->name, sizeof(uproc->name));
      safestrcpy(uproc->state, states[p->state], sizeof(uproc->state));
    }
  }
  release(&ptable.lock);

  return i;
}

int
setuid(struct proc* proc, uint uid)
{
  // uid is bounds checked in sysproc.c
  // Atomically set the process UID
  acquire(&ptable.lock);
  proc->uid = uid;
  release(&ptable.lock);

  return uid;
}

int
setgid(struct proc* proc, uint gid)
{
  // gid is bounds checked in sysproc.c
  // Atomically set the process GID
  acquire(&ptable.lock);
  proc->gid = gid;
  release(&ptable.lock);

  return gid;
}

#endif // CS333_P2

#if defined(CS333_P3)
// list management helper functions
static void
stateListAdd(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL){
    (*list).head = p;
    (*list).tail = p;
    p->next = NULL;
  } else{
    ((*list).tail)->next = p;
    (*list).tail = ((*list).tail)->next;
    ((*list).tail)->next = NULL;
  }
}
#endif

#if defined(CS333_P3)
static int
stateListRemove(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL || (*list).tail == NULL || p == NULL){
    return -1;
  }

  struct proc* current = (*list).head;
  struct proc* previous = 0;

  if(current == p){
    (*list).head = ((*list).head)->next;
    // prevent tail remaining assigned when we've removed the only item
    // on the list
    if((*list).tail == p){
      (*list).tail = NULL;
    }
    return 0;
  }

  while(current){
    if(current == p){
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found. return error
  if(current == NULL){
    return -1;
  }

  // Process found.
  if(current == (*list).tail){
    (*list).tail = previous;
    ((*list).tail)->next = NULL;
  } else{
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = NULL;

  return 0;
}
#endif

#if defined(CS333_P3)
static void
initProcessLists()
{
  int i;

  for (i = UNUSED; i <= ZOMBIE; i++) {
    ptable.list[i].head = NULL;
    ptable.list[i].tail = NULL;
  }
#if defined(CS333_P4)
  for (i = 0; i <= MAXPRIO; i++) {
    ptable.ready[i].head = NULL;
    ptable.ready[i].tail = NULL;
  }
#endif
}
#endif

#if defined(CS333_P3)
static void
initFreeList(void)
{
  struct proc* p;

  for(p = ptable.proc; p < ptable.proc + NPROC; ++p){
    p->state = UNUSED;
    stateListAdd(&ptable.list[UNUSED], p);
  }
}
#endif

#if defined(CS333_P3)
// example usage:
// assertState(p, UNUSED, __FUNCTION__, __LINE__);
// This code uses gcc preprocessor directives. For details, see
// https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
static void
assertState(struct proc *p, enum procstate state, const char * func, int line)
{
    if (p->state == state)
      return;
    cprintf("Error: proc state is %s and should be %s.\nCalled from %s line %d\n",
        states[p->state], states[state], func, line);
    panic("Error: Process state incorrect in assertState()");
}
#endif

#if defined(CS333_P3)
static void
__transition(enum procstate A, enum procstate B, struct proc* p, const char* func, int line)
{

#ifdef CS333_P4
  int rc = (A == RUNNABLE) ? stateListRemove(&ptable.ready[p->priority], p) 
                           : stateListRemove(&ptable.list[A], p);
#else
  int rc = stateListRemove(&ptable.list[A], p);
#endif // CS333_P4

  if(rc == -1) {
    cprintf("\n__transition: Called from %s, line %d\n", func, line);
    panic("Error: Failed to remove process from state list in transition");
  }

  assertState(p, A, func, line);
  
  p->state = B;

#ifdef CS333_P4
  (B == RUNNABLE) ? stateListAdd(&ptable.ready[p->priority], p) 
                  : stateListAdd(&ptable.list[B], p);
#else
  stateListAdd(&ptable.list[B], p);
#endif // CS333_P4
}

static void
__atom_transition(enum procstate A, enum procstate B, struct proc* p, const char* func, int line)
{
  acquire(&ptable.lock);
  __transition(A, B, p, func, line);
  release(&ptable.lock);
}

static void*
foldr(void* (*f)(struct proc*, void*), void* acc, struct proc* list)
{
  if(!list) { return acc; }

  return f(list, foldr(f, acc, list->next));
}

static void*
foldl(void* (*f)(void*, struct proc*), void* acc, struct proc* list)
{
  // TODO, figure out a way to conduct early termination of the fold
  if(!list) { return acc; }

  // Tail-recursive
  return foldl(f, f(acc, list), list->next);
}

static void
map(void (*f)(struct proc*), struct proc* list)
{
  foldl(LAMBDA(void* _(void* acc, struct proc* p){
    f(p);
    return acc;
  }), NULL, list);
}

// PRECONDITION - Assumes ptable lock is held by the callee
static struct proc* 
find_if(int (*predicate)(struct proc*))
{
  struct proc* p;
  for(enum procstate state = EMBRYO; state <= ZOMBIE; ++state)
  {
    for(p = ptable.list[state].head; p != NULL; p = p->next)
    {
      if(predicate(p))
        return p;
    }
  }
#ifdef CS333_P4
  // Search Ready List
  for(int i = 0; i <= MAXPRIO; ++i)
  {
    for(p = ptable.ready[i].head; p != NULL; p = p->next)
    {
      if(predicate(p))
        return p;
    }
  }
#endif //CS333_P4
  return NULL;
}

#endif // CS333_P3

#ifdef CS333_P4

// PRECONDITION - Assumes ptable lock is held by the callee
static void
adjust_priority(struct proc* p, int diff)
{
  int new_priority = p->priority + diff;
  
  // Don't set illegal priority
  if(new_priority > MAXPRIO || new_priority < 0 || diff == 0) { return; }

  if(p->state == RUNNABLE) // Update MLFQ ready lists
  {
    if(stateListRemove(&ptable.ready[p->priority], p) == -1)
      panic("adjust_priority: FATAL, failed to remove proc from state list");

    stateListAdd(&ptable.ready[new_priority], p);
  }
  p->priority = new_priority;
  p->budget   = DEFAULT_BUDGET;
}

// PRECONDITION - Assumes ptable lock is held by the callee
static void
promote_all_procs(uint levels)
{
  if(levels > MAXPRIO) { return; }

  for(int i = MAXPRIO - levels; i >= 0; --i)
  {
    map(LAMBDA(void _(struct proc* p){
      adjust_priority(p, levels);
    }), ptable.ready[i].head);
  }

  // Promote sleeping list
  map(LAMBDA(void _(struct proc* p){
    adjust_priority(p, levels);
  }), ptable.list[SLEEPING].head);

  // Promote Running list
  map(LAMBDA(void _(struct proc* p){
    adjust_priority(p, levels);
  }), ptable.list[RUNNING].head);
}

int 
setpriority(uint pid, uint priority)
{
  if(priority <= MAXPRIO)
  {
    acquire(&ptable.lock);

    struct proc* p = find_if(LAMBDA(int _(struct proc* p){
      return (p->pid == pid) ? 1 : 0;
    }));

    adjust_priority(p, priority - p->priority);
    release(&ptable.lock);
    return 0; // Success
  }
  return -1;
}

int
getpriority(uint pid)
{
  acquire(&ptable.lock);
  struct proc* p = find_if(LAMBDA(int _(struct proc* p){
    return (p->pid == pid) ? 1 : 0;
  }));
  release(&ptable.lock);

  return (p != NULL) ? p->priority : -1;
}

int
lowerbudget(uint pid, int diff)
{
  acquire(&ptable.lock);
  struct proc* p = find_if(LAMBDA(int _(struct proc* p){
    return (p->pid == pid) ? 1 : 0;
  }));

  if(p) 
  { 
    int new_budget = p->budget - diff;

    // Prevent underflow
    if(new_budget < p->budget) { p->budget = new_budget; }

    if(p->budget <= 0 && p->priority > 0)
      adjust_priority(p, -1);
  }

  release(&ptable.lock);

  return (p != NULL) ? 0 : -1;
}

#endif // CS333_P4;
