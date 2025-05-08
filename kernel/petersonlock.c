#include "petersonlock.h"
#include "param.h"
#include "defs.h"
#define PETERSONLOCK 15    // number of locks for Peterson's algorithm

struct petersonlock petersonlocks [PETERSONLOCK]; // array of Peterson locks

int peterson_destroy(int lock_id) {
    // Validate the lock_id
    if (lock_id < 0 || lock_id >= PETERSONLOCK)
        return -1; // invalid lock id
    if (petersonlocks[lock_id].created == 0)
        return -1; // lock not created

    // Initialize the lock
    struct petersonlock *lk = &petersonlocks[lock_id];
    lk->created = 0;
    lk->lock = 0;
    lk->flags[0] = 0;
    lk->flags[1] = 0;
    return 0; // success
}

// Initialize the Peterson locks
void initpeterson() {
  for (int i = 0; i < PETERSONLOCK; i++) {
    peterson_destroy(i);
  }
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

int peterson_acquire(int lock_id, int role) {
    // Validate the lock_id and role
    if (role < 0 || role > 1)
        return -1; // invalid role
    if (lock_id < 0 || lock_id >= PETERSONLOCK)
        return -1; // invalid lock id
    if (petersonlocks[lock_id].created == 0)
        return -1; // lock not created 
    struct petersonlock *lk = &petersonlocks[lock_id];

    // Peterson's algorithm for mutual exclusion
    while (1) {
        __sync_synchronize(); // Ensure memory synchronization
        if (__sync_lock_test_and_set(&lk->lock, 1) == 0) {
            // Successfully acquired the lock
            __sync_synchronize(); // Ensure memory synchronization
            lk->flags[role] = 1; // Set the flag for the current role
            __sync_synchronize(); // Ensure memory synchronization
        }
        if (!lk->flags[role]) {
            yield(); // Give up the CPU and retry
        } else {
            // Successfully acquired the lock for the current role
            break;
        }
    }
    return 0; // success
}

int peterson_release(int lock_id, int role) {
    // Validate the lock_id and role
    if (role < 0 || role > 1)
        return -1; // invalid role
    if (lock_id < 0 || lock_id >= PETERSONLOCK)
        return -1; // invalid lock id
    if (petersonlocks[lock_id].created == 0)
        return -1; // lock not created 
    struct petersonlock *lk = &petersonlocks[lock_id];

    // Release the lock
    if (lk->flags[role]) {
        lk->flags[role] = 0; // Clear the flag for the current role
        __sync_synchronize(); // Ensure memory synchronization
        __sync_lock_release(&lk->lock); // Release the lock
        __sync_synchronize(); // Ensure memory synchronization

    }
    return 0; // success
}






