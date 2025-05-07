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
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
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
  uint64 p;
  argaddr(0, &p);
  return wait(p);
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

// peterson lock system calls
uint64
sys_peterson_create(void)
{
  // TODO: implement
  return peterson_create();
}

uint64
sys_peterson_acquire(void)
{
  // TODO: implement
  int lock_id;
  int role;
  argint(0, &lock_id);
  argint(1, &role);
  if (lock_id < 0 || lock_id >= NLOCKS)
    return -1;
  if (role < 0 || role >= NLOCKS)
    return -1;
  return peterson_acquire(lock_id, role);
}

uint64
sys_peterson_release(void)
{
  // TODO: implement
  int n;
  if(argint(0, &n) < 0)
    return -1;
  return peterson_release(n);
}

uint64
sys_peterson_destroy(void)
{
  // TODO: implement
  int n;
  if(argint(0, &n) < 0)
    return -1;
  return peterson_destroy(n);
}

