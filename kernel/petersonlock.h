#include "types.h"
#include <stdbool.h>

struct petersonlock {
    bool created; // Indicates if the lock is active
    uint lock; // role 0: -1 / role 1: 1 / free: 0
    bool flags [2]; // flags for each role
};