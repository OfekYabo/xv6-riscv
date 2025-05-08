// kernel/peterson.h

#ifndef PETERSON_H
#define PETERSON_H

#define MAX_PETERSON_LOCKS 15

struct peterson_lock {
  int used;         // 0 = unused, 1 = used
  int flag[2];      // Peterson flags for each process
  int turn;         // Whose turn it is
};

#endif
