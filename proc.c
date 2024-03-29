#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "sysproc.h"
#define SENTINEL_VALUE 32767 // maximum value of an integer
#define MAX_NUM_BURSTS 100 // size of burst array
#define SIZE_OF_BURST_AVG 3 // determines how many bursts are averaged in SJRF scheduler
#define MAX_NUM_LOCKS 100

typedef int pid_t;
typedef pid_t tid_t;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct thread_spin_lock thread_locks[MAX_NUM_LOCKS];
int nextLockId = 1;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
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

  // initialize burst metadata
  p->initial_burst = sys_uptime();
  p->sburst = p->initial_burst;
  p->burst_idx = 0;
  memset(p->burstarr, 0x0, MAX_NUM_BURSTS * sizeof(int));

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
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

  p->state = RUNNABLE;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
pid_t
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;


  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
	      // clean up burst metadata
	      p->burst_idx = 0;
	      p->sburst = 0;
        memset(p->burstarr, 0x0, MAX_NUM_BURSTS * sizeof(int));
	      // finished cleaning bursts
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

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

   for(;;){
     // Enable interrupts on this processor.
     sti();

     // Loop over process table looking for process to run.
     acquire(&ptable.lock);
     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
       if(p->state != RUNNABLE)
         continue;

       // Switch to chosen process.  It is the process's job
       // to release ptable.lock and then reacquire it
       // before jumping back to us.
       proc = p;
       switchuvm(p);
       p->state = RUNNING;
       swtch(&cpu->scheduler, proc->context);
       switchkvm();

       // Process is done running for now.
       // It should have changed its p->state before coming back.
       proc = 0;
     }
     release(&ptable.lock);

   }
 }


/*  MP1 FOR CS450 SCHEDULER **********************
// helper method for SJRF scheduler
static int
less_than_three_bursts(struct proc *p) {
  if(p->burst_idx <=2 && p->burstarr[3] == 0x0)
    return 1;
  return 0;
}

void
scheduler(void)
{
  struct proc *p;
  struct proc *min_p = 0;
  for(;;) {
    // Enable interrupts on this processor.
    sti();
    int min_avg = SENTINEL_VALUE;
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state != RUNNABLE)
          continue;

          if (less_than_three_bursts(p)) { // do RR scheduling
            proc = p;
            switchuvm(p);
            p->state = RUNNING;
            swtch(&cpu->scheduler, proc->context);
            switchkvm(); // preempts the running process
            proc = 0;
          } // end RR scheduling

          else { // do SJRF scheduling
            int burstIndex;
            int loop_counter = SIZE_OF_BURST_AVG;
            int burst_sum = 0;
            int avg_burst_time;

            // calculate the average burst times for each proc
            for (burstIndex = p->burst_idx; loop_counter >=0; loop_counter --, burstIndex--) {
              if (burstIndex < 0) { // edge case for burst array
                burstIndex = MAX_NUM_BURSTS - 1;
              }
              burst_sum += p->burstarr[burstIndex];
            } // end for
            avg_burst_time = burst_sum / SIZE_OF_BURST_AVG;

            // save a pointer to the process with the minimum average burst time
            if (avg_burst_time < min_avg) {
              min_avg = avg_burst_time;
              min_p = p;
            }

            // schedule the shortest job and let it run to completion
            if (min_p->state == RUNNABLE) {
              proc = min_p;
              switchuvm(min_p);
              min_p->state = RUNNING;
              swtch(&cpu->scheduler, proc->context);

              //switchkvm(); this should not be called in SJRF.

              proc = 0;
              min_p = 0;
            }
          } // end SJRF scheduling
        } // end ptable 'for loop'
      release(&ptable.lock);
    }// end ;; 'for loop'
} // end scheduler

***************MP1 SCHEDULER FOR CS450*****************/

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

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
    initlog();
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

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

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// Threading API for MP2 IIT CS 450 follows
//****************************THREADING***************************

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
/*
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
*/

/*
  create the stack frame for the thread execution and return
  pointer to the top of the newly created stack
  MUST PASS IN SPACE ALLOCATED BY MALLOC as the "*stack" parameter
*/


static uint
thread_stack_init(void *stack, void *args)
{
  // the stack will be represented by an array
  // Stack has LIFO (Last In First Out) push-pop
  //  (stack top)
  //  --------
  //  arg(int)
  //  --------
  //  0
  //  --------
  //  address of arg
  //  --------
  //  return PC
  //  --------
  //(stack bottom)

  uint user_stack[3];
  uint stack_p = (uint) stack;
  stack_p = (stack_p - sizeof(int)) & ~3;
  copyout(proc->pgdir, stack_p, args, sizeof(int));
  user_stack[2] = 0;
  user_stack[1] = stack_p;
  user_stack[0] = 0xFFFFFFFF;
  stack_p -= 3* sizeof(int);
  copyout(proc->pgdir, stack_p, user_stack, 3*4);
  return stack_p;
}


tid_t
thread_create(void (*tmain)(void *), void *stack, void *arg)
{
  tid_t tid;
  struct proc *np;

  // Allocate thread process
  if((np = allocproc()) == 0) {
    	return -1;
  }

  np->pgdir = proc->pgdir;
  np->sz = proc->sz;
  np->parent = proc;

  // init trap frame
  memset(np->tf, 0, sizeof(*np->tf));
  *np->tf = *proc->tf;

  // init the stack (esp = stack pointer)
  np->thread_stack_top = stack;
  np->tf->esp = thread_stack_init(stack, arg);

  // set the PC of the new thread process  (eip = PC)
  np->tf->eip = (uint) tmain;

  // Clear %eax so that thread_create returns 0 in the children
  np->tf->eax = 0;

  int i;
  for(i = 0; i < NOFILE; i++)
    	if(proc->ofile[i])
      		np->ofile[i] = filedup(proc->ofile[i]);
  		np->cwd = idup(proc->cwd);

  tid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return tid;
}


tid_t
thread_join(void **stack)
{
  struct proc *p;
  int havekids;
  tid_t tid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->parent != proc)
      continue;
    if (p->pgdir != proc->pgdir)
      continue;
    havekids = 1;
    if(p->state == ZOMBIE) {
      // Found one.
      tid = p->pid;
      kfree(p->kstack);
      p->kstack = 0;
      // freevm(p->pgdir);
      p->state = UNUSED;
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      // clean up burst metadata
      p->burst_idx = 0;
      p->sburst = 0;
      memset(p->burstarr, 0x0, MAX_NUM_BURSTS * sizeof(int));
      // finished cleaning bursts
      *stack = p->thread_stack_top;
      release(&ptable.lock);
      return tid;
    }
  }

  // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
