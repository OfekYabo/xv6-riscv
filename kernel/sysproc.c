#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "petersonlock.h"


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
extern struct spinlock peterson_lock_table_lock;
extern struct peterson_lock peterson_locks[];

uint64
sys_peterson_create(void)
{
  acquire(&peterson_lock_table_lock);

  for (int i = 0; i < MAX_PETERSON_LOCKS; i++) {
    if (peterson_locks[i].used == 0) {
      peterson_locks[i].used = 1;
      peterson_locks[i].flag[0] = 0;
      peterson_locks[i].flag[1] = 0;
      peterson_locks[i].turn = 0;
      release(&peterson_lock_table_lock);
      return i;
    }
  }

  release(&peterson_lock_table_lock);
  return -1;  // No free lock found
}

uint64
sys_peterson_acquire(void)
{
  int lock_id, role;

  // Extract syscall arguments (no return value from argint!)
  argint(0, &lock_id);
  argint(1, &role);

  // Validate arguments
  if (lock_id < 0 || lock_id >= MAX_PETERSON_LOCKS)
    return -1;
  if (role != 0 && role != 1)
    return -1;

  struct peterson_lock *lock = &peterson_locks[lock_id];

  acquire(&peterson_lock_table_lock);
  if (!lock->used) {
    release(&peterson_lock_table_lock);
    return -1;
  }
  release(&peterson_lock_table_lock);

  // Begin Peterson lock protocol
  __sync_lock_test_and_set(&lock->flag[role], 1);  // indicate interest
  __sync_synchronize();                            // memory barrier

  __sync_lock_test_and_set(&lock->turn, 1 - role); // yield to the other
  __sync_synchronize();

  // Wait until the other process is not interested or it's our turn
  while (lock->flag[1 - role] && lock->turn == 1 - role) {
    yield();  // voluntarily give up the CPU
  }

  return 0;
}


uint64
sys_peterson_release(void)
{
  int lock_id, role;

  // Retrieve syscall arguments
  argint(0, &lock_id);
  argint(1, &role);

  // Validate inputs
  if (lock_id < 0 || lock_id >= MAX_PETERSON_LOCKS)
    return -1;
  if (role != 0 && role != 1)
    return -1;

  struct peterson_lock *lock = &peterson_locks[lock_id];

  acquire(&peterson_lock_table_lock);
  if (!lock->used) {
    release(&peterson_lock_table_lock);
    return -1;
  }
  release(&peterson_lock_table_lock);

  // Release the lock
  __sync_lock_release(&lock->flag[role]);
  __sync_synchronize();  // Ensure memory ordering

  return 0;
}


uint64
sys_peterson_destroy(void)
{
  int lock_id;

  argint(0, &lock_id);

  // Validate input
  if (lock_id < 0 || lock_id >= MAX_PETERSON_LOCKS)
    return -1;

  struct peterson_lock *lock = &peterson_locks[lock_id];

  acquire(&peterson_lock_table_lock);
  if (!lock->used) {
    release(&peterson_lock_table_lock);
    return -1;
  }

  // Mark lock as free and clear state
  lock->used = 0;
  lock->flag[0] = 0;
  lock->flag[1] = 0;
  lock->turn = 0;

  release(&peterson_lock_table_lock);
  return 0;
}

