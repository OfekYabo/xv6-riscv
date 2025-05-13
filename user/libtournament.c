#include "user.h"
#include "stddef.h"
#include "user.h"

#define MAX_PROCESSES 16
#define MAX_LEVELS 4  // log2(16)

static int proc_index = -1;                 // process index (0 to N-1)
static int num_levels = 0;                  // how many levels in the tree



#define MAX_PROCS 16
#define MAX_LEVELS 4  // log2(16) = 4
static int all_lock_ids[MAX_PROCS - 1]; // MAX_PROCS is enough (won't exceed 15)


// Declare the system calls manually
int peterson_create(void);
int peterson_acquire(int lock_id, int role);
int peterson_release(int lock_id, int role);
int peterson_destroy(int lock_id);

int tournament_id = -1;
int num_procs = 0;
int levels = 0;

int lock_ids[MAX_PROCS][MAX_LEVELS]; // locks for each process at each level
int roles[MAX_PROCS][MAX_LEVELS];    // role (0 or 1) for each process at each level

int
tournament_create(int processes) {
    if (processes < 1 || processes > MAX_PROCESSES)
        return -1;

    // Check if processes is a power of 2
    if ((processes & (processes - 1)) != 0)
        return -1;

    num_levels = 0;
    int temp = processes;
    while (temp >>= 1) num_levels++;

    int total_locks = processes - 1; // Total internal nodes in a binary tree


    // Create required locks
    for (int i = 0; i < total_locks; i++) {
        int id = peterson_create();
        if (id < 0) return -1;
        all_lock_ids[i] = id;
    }

    // Assign process indices
    for (int i = 1; i < processes; i++) {
        int pid = fork();
        if (pid == 0) {
            proc_index = i;
            break;
        } else if (pid < 0) {
            return -1;
        }
    }

    if (proc_index == -1) proc_index = 0;

    // Calculate roles and lock_ids for each level
    for (int l = 0; l < num_levels; l++) {
        int role_mask = 1 << (num_levels - l - 1);
        roles[proc_index][l] = (proc_index & role_mask) ? 1 : 0;

        int lockl = proc_index >> (num_levels - l);
        int base_index = (1 << l) - 1;
        lock_ids[proc_index][l] = all_lock_ids[base_index + lockl];
    }

    return proc_index;
}


/* acquire from leaf-to-root */
int
tournament_acquire(void)
{
  for (int l = num_levels - 1; l >= 0; l--) {          // ↓ bottom → top
    if (peterson_acquire(lock_ids[proc_index][l],
                         roles[proc_index][l]) < 0)
      return -1;
  }
  return 0;
}

/* release from root-to-leaf */
int
tournament_release(void)
{
  for (int l = 0; l < num_levels; l++) {               // ↑ top → bottom
    if (peterson_release(lock_ids[proc_index][l],
                         roles[proc_index][l]) < 0)
      return -1;
  }
  return 0;
}


