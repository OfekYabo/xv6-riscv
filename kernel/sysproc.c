#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
    int status;
    char exit_msg[32]; // Temporary buffer for the exit message

    // Retrieve the status argument
    argint(0, &status);

    // Retrieve the exit message argument
    if (argstr(1, exit_msg, sizeof(exit_msg)) < 0)
        return -1;

    // Save the exit message in the process's PCB
    struct proc *p = myproc();
    safestrcpy(p->exit_msg, exit_msg, sizeof(p->exit_msg));

    // Call the actual exit function
    exit(status);
    return 0; // This line will never be reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
    uint64 status_addr, msg_addr;

    // Retrieve the status pointer
    argaddr(0, &status_addr);

    // Retrieve the exit message pointer
    argaddr(1, &msg_addr);

    // Call the actual wait function
    return wait(status_addr, msg_addr);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
 sys_memsize(void)
 {
     struct proc *p = myproc();
     return p->sz;  // 'sz' is the process size in bytes
 }

