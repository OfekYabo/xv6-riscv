#include "petersonlock.h"
#include "param.h"

struct petersonlock petersonlocks [PETERSONLOCK];

// Initialize the Peterson locks
void initpeterson() {
  for (int i = 0; i < PETERSONLOCK; i++) {
    initpetersonlock(&petersonlocks[i]);
  }
}

void initpetersonlock(struct petersonlock *lk) {
    lk->created = 0;
    lk->lock = 0;
    lk->flags[0] = false;
    lk->flags[1] = false;
}

int peterson_create(void) {
    for (int i = 0; i < PETERSONLOCK; i++) {
        if (!petersonlocks[i].created) {
            petersonlocks[i].created = 1;
            return i;
        }
    }
    return -1; // no available lock
}

void peterson_acquire(struct petersonlock *lk, int role) {
    while (1) {
        __sync_synchronize(); // Ensure memory synchronization
        if (__sync_lock_test_and_set(&lk->lock, 1) == 0) {
            // Successfully acquired the lock
            __sync_synchronize(); // Ensure memory synchronization
            lk->flags[role] = true; // Set the flag for the current role
            __sync_synchronize(); // Ensure memory synchronization
        }
        if (!lk->flags[role]) {
            yield(); // Give up the CPU and retry
        } else {
            // Successfully acquired the lock for the current role
            break;
        }
    }
}

void peterson_release(struct petersonlock *lk, int role) {
    if (lk->flags[role]) {
        lk->flags[role] = false; // Clear the flag for the current role
        __sync_synchronize(); // Ensure memory synchronization
        __sync_lock_release(&lk->lock); // Release the lock
        __sync_synchronize(); // Ensure memory synchronization

    }
}






