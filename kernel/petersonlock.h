#include "types.h"

struct petersonlock {
    uint created; // Indicates if the lock is active
    uint lock; // 0: unlocked, 1: locked
    uint flags [2]; // flags for each role
};