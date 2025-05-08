#include "types.h"
#include "param.h"
#include "petersonlock.h"
#include "spinlock.h"

struct peterson_lock peterson_locks[MAX_PETERSON_LOCKS];
struct spinlock peterson_lock_table_lock;
