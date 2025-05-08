#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "petersonlock.h"


volatile static int started = 0;
extern struct spinlock peterson_lock_table_lock;
extern struct peterson_lock peterson_locks[];

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    //initppeterson();
    initlock(&peterson_lock_table_lock, "peterson_table");
    for (int i = 0; i < MAX_PETERSON_LOCKS; i++) {
      peterson_locks[i].used = 0;
      peterson_locks[i].flag[0] = 0;
      peterson_locks[i].flag[1] = 0;
      peterson_locks[i].turn = 0;
    }
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode table
    fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
