#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

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
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
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
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_start_burst(void)
{
  uint start_ticks = sys_uptime();
  proc->sburst = start_ticks;
  proc->burst_idx++; // store current burst in next postition in burstarr
  if (proc->burst_idx > 100)
    proc->burst_idx = 0; // edge case
  return start_ticks;
}

int
sys_end_burst(void)
{
  uint end_ticks = sys_uptime();
  uint burst = end_ticks - proc->sburst;
  proc->burstarr[proc->burst_idx] = burst;
  return end_ticks;
}

int
sys_print_bursts(void)
{
  // first end the turnaround timer
  proc->turnaround = sys_uptime() - proc->initial_burst;

  // print out process bursts
  int i;
  for (i=0; i < 100; i++) {
    if (proc->burstarr[i] != 0x0)
      cprintf("%d, ", proc->burstarr[i]);
  }
  cprintf("turnaround time: %d\n", proc->turnaround);
return 0;
}

int
sys_thread_create(void)
{
  /*
  int tmain; // the function being called by the thread
  char *stack;
  char *arg;

  if (argint(0, &tmain) < 0)
    return -1;
  if (argint(1, &stack) < 0)
    return -1;
  if (argint(2, &arg) < 0)
    return -1;
  */
  void (*tmain)(void *);
  char *stack;
  char *arg;

  argptr(0, (char**)&tmain, 1);
  argstr(1, &stack);
  argstr(2, &arg);

  return thread_create((void *)tmain, (void *)stack, (void *)arg);
}

int
sys_thread_join(void)
{
  int stack;

  if (argint(0, &stack) < 0)
    return -1;

  return thread_join((void **) stack);
}

extern int nextLockId;
extern struct thread_spin_lock thread_locks[100];

int
sys_mtx_create(void)
{
	int locked;
	argint(0, &locked);
	int lockId = nextLockId++;
	xchg(&(thread_locks[lockId].locked), locked);
	return lockId;
}

int
sys_mtx_lock(void)
{
	int lockId;
	argint(0, &lockId);
	while(xchg(&(thread_locks[lockId].locked), 1) != 0);
	return 0;
}

int
sys_mtx_unlock(void)
{
	int lockId;
	argint(0, &lockId);
	xchg(&(thread_locks[lockId].locked), 0);
	return 0;
}
