#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define PAGESIZE 4096

/* Return 1 if -nounmap flag is present */
static int
nounmap_flag(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
    if (strcmp(argv[i], "-nounmap") == 0)
      return 1;
  return 0;
}

int
main(int argc, char *argv[])
{
  int nounmap = nounmap_flag(argc, argv);

  /* pipes:
       p2c : parent→child  (send parent-PID)
       a2p : child →parent (send addr_in_parent)
  */
  int p2c[2], a2p[2];
  pipe(p2c);
  pipe(a2p);

  int pid = fork();
  if (pid < 0) { printf("fork failed\n"); exit(1); }

  /* ---------------------------------------------------------------- child */
  if (pid == 0) {
    close(p2c[1]); close(a2p[0]);

    /* get parent PID */
    int parent_pid;
    read(p2c[0], &parent_pid, sizeof(parent_pid));
    close(p2c[0]);

    /* allocate one page, write message */
    char *buf = malloc(PAGESIZE);
    if (!buf) { printf("child malloc failed\n"); exit(1); }
    strcpy(buf, "Hello daddy");

    /* map my page into parent; syscall returns addr inside parent */
    uint64 addr_in_parent = map_shared_pages(buf, PAGESIZE, parent_pid);
    if (addr_in_parent == 0) { printf("child map failed\n"); exit(1); }

    /* send that address to parent */
    write(a2p[1], &addr_in_parent, sizeof(addr_in_parent));
    close(a2p[1]);

    /* child is done; it never unmaps (requirement 6 tests kernel safety) */
    exit(0);
  }

  /* --------------------------------------------------------------- parent */
  else {
    /* Close pipe ends we don't use */
    close(p2c[0]);      // parent won't read from p2c
    close(a2p[1]);      // parent won't write to a2p

    /* Measure size *before* anything happens */
    uint64 sz_before = (uint64)sbrk(0);

    /* 1. Send my PID to the child */
    int mypid = getpid();
    write(p2c[1], &mypid, sizeof(mypid));
    close(p2c[1]);

    /* 2. Receive the address the child mapped for me */
    uint64 shared_addr;
    if (read(a2p[0], &shared_addr, sizeof(shared_addr))
        != sizeof(shared_addr)) {
      printf("parent: failed to read address\n");
      exit(1);
    }
    close(a2p[0]);

    /* 3. Size right after mapping (child already did map in kernel) */
    uint64 sz_after_map = (uint64)sbrk(0);

    /* 4. Use the shared buffer while it is still mapped */
    printf("Parent reads from shared memory: %s\n", (char *)shared_addr);

    /* 5. Optional unmap + malloc bookkeeping */
    uint64 sz_after_unmap  = sz_after_map;
    uint64 sz_after_malloc = sz_after_map;

    if (!nounmap) {
      /* Un-map the shared page from my address space */
      unmap_shared_pages((void *)shared_addr, PAGESIZE);
      sz_after_unmap = (uint64)sbrk(0);          // should shrink

      /* Allocate a new page to prove heap reuse */
      malloc(PAGESIZE);
      sz_after_malloc = (uint64)sbrk(0);         // grows back
    }

    /* 6. Print the size progression */
    printf("parent sz: before=%p afterMap=%p "
           "afterUnmap=%p afterMalloc=%p\n",
           (void*)sz_before,
           (void*)sz_after_map,
           (void*)sz_after_unmap,
           (void*)sz_after_malloc);

    /* 7. Clean up */
    wait(0);   // reap child
    exit(0);
  }
}