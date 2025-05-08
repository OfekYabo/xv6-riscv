#include "petersonlock.h"
#include "param.h"
#include "defs.h"

struct petersonlock petersonlocks [PETERSONLOCK];

uint64 initpetersonlock(int lock_id) {
    if (petersonlocks[lock_id].created == 0)
        return -1; // lock not created 
    struct petersonlock lk = petersonlocks[lock_id];
    lk.created = 0;
    lk.lock = 0;
    lk.flags[0] = 0;
    lk.flags[1] = 0;
    return 0; // success
}

// Initialize the Peterson locks
void initpeterson() {
  for (int i = 0; i < PETERSONLOCK; i++) {
    initpetersonlock(i);
  }
}

uint64 peterson_create(void) {
    for (int i = 0; i < PETERSONLOCK; i++) {
        if (!petersonlocks[i].created) {
            petersonlocks[i].created = 1;
            return i;
        }
    }
    return -1; // no available lock
}

uint64 peterson_acquire(int lock_id, int role) {
    if (petersonlocks[lock_id].created == 0)
        return -1; // lock not created 
    struct petersonlock lk = petersonlocks[lock_id];
    while (1) {
        __sync_synchronize(); // Ensure memory synchronization
        if (__sync_lock_test_and_set(&lk.lock, 1) == 0) {
            // Successfully acquired the lock
            __sync_synchronize(); // Ensure memory synchronization
            lk.flags[role] = 1; // Set the flag for the current role
            __sync_synchronize(); // Ensure memory synchronization
        }
        if (!lk.flags[role]) {
            yield(); // Give up the CPU and retry
        } else {
            // Successfully acquired the lock for the current role
            break;
        }
    }
    return 0; // success
}

uint64 peterson_release(int lock_id, int role) {
    if (petersonlocks[lock_id].created == 0)
        return -1; // lock not created 
    struct petersonlock lk = petersonlocks[lock_id];
    if (lk.flags[role]) {
        lk.flags[role] = 0; // Clear the flag for the current role
        __sync_synchronize(); // Ensure memory synchronization
        __sync_lock_release(&lk.lock); // Release the lock
        __sync_synchronize(); // Ensure memory synchronization

    }
    return 0; // success
}






