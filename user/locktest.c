#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define ITERATIONS 100

int
main(void)
{
  int lock_id = peterson_create();
  if (lock_id < 0) {
    printf("Failed to create lock\n");
    exit(1);
  }

  int fork_ret = fork();
  int role = fork_ret > 0 ? 0 : 1;

  for (int i = 0; i < ITERATIONS; i++) {
    if (peterson_acquire(lock_id, role) < 0) {
      printf("Failed to acquire lock\n");
      exit(1);
    }

    // Critical section
    if (role == 0)
      printf("Parent process in critical section\n");
    else
      printf("Child process in critical section\n");

    if (peterson_release(lock_id, role) < 0) {
      printf("Failed to release lock\n");
      exit(1);
    }
  }

  if (fork_ret > 0) {
    wait(0);
    printf("Parent process destroying lock\n");
    if (peterson_destroy(lock_id) < 0) {
      printf("Failed to destroy lock\n");
      exit(1);
    }
  }

  exit(0);
}
