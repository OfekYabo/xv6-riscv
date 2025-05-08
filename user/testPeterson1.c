#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    // Create a Peterson lock
    int lock_id = peterson_create();
    if (lock_id < 0) {
        printf("Failed to create lock\n");
        exit(1);
    }

    // Fork a child process
    int fork_ret = fork();
    int role = fork_ret > 0 ? 0 : 1; // Parent gets role 0, child gets role 1

    for (int i = 0; i < 100; i++) {
        // Acquire the lock
        if (peterson_acquire(lock_id, role) < 0) {
            printf("Failed to acquire lock\n");
            exit(1);
        }

        // Critical section
        if (role == 0) {
            printf("Parent process in critical section\n");
        } else {
            printf("Child process in critical section\n");
        }

        // Release the lock
        if (peterson_release(lock_id, role) < 0) {
            printf("Failed to release lock\n");
            exit(1);
        }
    }

    // Parent process destroys the lock after the child finishes
    if (fork_ret > 0) {
        wait(0); // Wait for the child to finish
        printf("Parent process destroying lock\n");
        if (peterson_destroy(lock_id) < 0) {
            printf("Failed to destroy lock\n");
            exit(1);
        }
    }

    exit(0);
}