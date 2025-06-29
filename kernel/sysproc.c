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

// System call: map_shared_pages
// Maps a region of memory from the calling process into another process as shared memory.
// Arguments (from user):
//   arg0: src_va (virtual address in source process)
//   arg1: size (number of bytes to map)
//   arg2: dst_pid (PID of destination process)
// Returns: virtual address in destination process, or -1 on error
uint64
sys_map_shared_pages(void) {
  uint64 src_va;
  int size, dst_pid;

  // Extract arguments from user
  argaddr(0, &src_va);
  argint(1, &size);
  argint(2, &dst_pid);

  // Find the destination process by PID
  struct proc *dst_proc = find_proc_by_pid(dst_pid);
  if (!dst_proc)
    return -1; // Error: destination process not found

  // Call kernel function to perform mapping
  return map_shared_pages(myproc(), dst_proc, src_va, size);
}

// System call: unmap_shared_pages
// Unmaps a region of shared memory from the calling process.
// Arguments (from user):
//   arg0: addr (virtual address to unmap)
//   arg1: size (number of bytes to unmap)
// Returns: 0 on success, -1 on error
uint64
sys_unmap_shared_pages(void) {
  uint64 addr;
  int size;

  // Extract arguments from user
  argaddr(0, &addr);
  argint(1, &size);

  // Call kernel function to perform unmapping
  return unmap_shared_pages(myproc(), addr, size);
}


