#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define PGSIZE        4096
#define MAX_CHILDREN  10
#define MSG_LEN       64

// Header for each log message in the shared buffer
struct log_header {
  uint16 msg_len;    // LOW half-word: length of the message
  uint16 child_idx;  // HIGH half-word: index of the child that wrote the message
};

/* int → decimal ASCII (xv6 has no sprintf) */
static void
itoa(int n, char *buf)
{
  int i = 0;
  if (n == 0) { buf[i++] = '0'; buf[i] = 0; return; }
  while (n) { buf[i++] = '0' + n % 10; n /= 10; }
  buf[i] = 0;
  for (int j = 0; j < i/2; j++) { char t = buf[j]; buf[j]=buf[i-1-j]; buf[i-1-j]=t; }
}

int
main(void)
{
  // Parent allocates a page-aligned buffer for logging (shared memory)
  uint64 cur = (uint64)sbrk(0);
  uint64 base = (cur + PGSIZE - 1) & ~(PGSIZE - 1);   // round up to page boundary
  sbrk(base - cur + PGSIZE);                          // ensure at least one page is allocated
  char *logbuf = (char *)base;
  memset(logbuf, 0, PGSIZE);                          // zero out the buffer (all headers = 0)

  // One pipe per child to send the mapped address (parent to child)
  int p2c[MAX_CHILDREN][2];
  int child_pid[MAX_CHILDREN];

  // Fork children
  for (int i = 0; i < MAX_CHILDREN; i++) {
    pipe(p2c[i]);
    int idx = i;
    int pid = fork();
    if (pid == 0) {                       // Child process
      close(p2c[idx][1]);                 // Only read from pipe

      uint64 shared;
      int n = read(p2c[idx][0], &shared, sizeof(shared)); // Receive mapped address from parent
      if (n != sizeof(shared)) exit(1);
      close(p2c[idx][0]);

      uint64 page_end = (shared & ~(PGSIZE - 1)) + PGSIZE; // End of the shared page

      // Build the log message (include child index)
      char msg[MSG_LEN];
      char num[8];
      itoa(idx, num);
      strcpy(msg, "Hello from child ");
      strcpy(msg + strlen(msg), num);
      int len = strlen(msg);

      // Find the first 4-byte aligned header in the shared page
      uint8 *p = (uint8 *)(((shared + 3) & ~3));

      // Try to find a free slot in the buffer for the message
      while ((uint64)p + sizeof(struct log_header) + len <= page_end) {
        struct log_header *h = (struct log_header *)p;
        uint32 packed = (idx << 16) | len; // Pack child index and message length
        // Atomically claim the slot if it is free (header == 0)
        if (__sync_val_compare_and_swap((uint32 *)h, 0, packed) == 0) {
          // Slot claimed: copy the message after the header
          memmove(p + sizeof(struct log_header), msg, len);
          break;
        }
        // Slot occupied: skip to the next possible header
        uint16 skip = h->msg_len;
        if (skip == 0) skip = 1;                     // If corrupt, move 1 byte
        p += sizeof(struct log_header) + skip;
        p  = (uint8 *)(((uint64)p + 3) & ~3);        // Align to next 4-byte boundary
      }
      exit(0); // Child done
    }
    // Parent after fork: close read end, save child pid
    close(p2c[i][0]);
    child_pid[i] = pid;
  }

  // Parent: map the buffer into each child and send the mapped address
  for (int i = 0; i < MAX_CHILDREN; i++) {
    uint64 addr_in_child = map_shared_pages(logbuf, PGSIZE, child_pid[i]);
    write(p2c[i][1], &addr_in_child, sizeof(addr_in_child));
    close(p2c[i][1]);
  }

  // Parent: poll the buffer and print messages as they appear, concurrently with children
  uint64 page_end = base + PGSIZE;
  uint8 *scan_start = (uint8 *)(((base + 3) & ~3)); // First aligned header
  int messages_seen[MAX_CHILDREN] = {0}; // Track messages per child (optional, for demo)
  int total_seen = 0;
  int expected = MAX_CHILDREN; // Expect one message per child
  while (total_seen < expected) {
    uint8 *p = scan_start;
    while ((uint64)p + sizeof(struct log_header) < page_end) {
      struct log_header *h = (struct log_header *)p;
      uint16 len = h->msg_len;
      uint16 idx = h->child_idx;
      if (len && !messages_seen[idx]) {
        if ((uint64)p + sizeof(struct log_header) + len > page_end) break;
        char buf[MSG_LEN+1];
        memmove(buf, p + sizeof(struct log_header), len);
        buf[len] = 0;
        printf("Parent read from child %d: %s\n", idx, buf);
        messages_seen[idx] = 1;
        total_seen++;
      }
      p += (len ? sizeof(struct log_header) + len : sizeof(struct log_header));
      p = (uint8 *)(((uint64)p + 3) & ~3); // Align to next 4-byte boundary
    }
    sleep(1); // Yield to children, avoid busy-wait
  }

  // Wait for all children to finish
  for (int i = 0; i < MAX_CHILDREN; i++) wait(0);

  exit(0);
}
