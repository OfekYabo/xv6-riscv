#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: tournament <num_processes>\n");
    exit(1);
  }

  int num = atoi(argv[1]);
  if (num < 1 || num > 16 || (num & (num - 1)) != 0) {
    printf("Error: num_processes must be a power of 2, up to 16\n");
    exit(1);
  }

  int tid = tournament_create(num);
  if (tid < 0) {
    printf("Error: tournament_create failed\n");
    exit(1);
  }

  /* parent waits for all children once, after fork tree is built */
  if (tid == 0) {
    for (int i = 0; i < num - 1; i++)
      wait(0);
  }

  /* -------- critical-section sequence, protected by the tournament lock -------- */
  if (tournament_acquire() < 0) {
    printf("Process %d (TID %d): failed to acquire\n", getpid(), tid);
    exit(1);
  }

  printf("Process %d (TID %d): in critical section\n", getpid(), tid);

  if (tournament_release() < 0) {
    printf("Process %d (TID %d): failed to release\n", getpid(), tid);
    exit(1);
  }
  /* ----------------------------------------------------------------------------- */

  exit(0);
}
