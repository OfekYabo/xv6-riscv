#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define PGSIZE        4096
#define MAX_CHILDREN  4
#define MSG_LEN       64

struct log_header {
  uint16 msg_len;    // LOW half-word
  uint16 child_idx;  // HIGH half-word
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
  /* ---------- parent allocates *page-aligned* buffer ---------- */
  uint64 cur = (uint64)sbrk(0);
  uint64 base = (cur + PGSIZE - 1) & ~(PGSIZE - 1);   // round up to page
  sbrk(base - cur + PGSIZE);                          // grow by at least one page
  char *logbuf = (char *)base;
  memset(logbuf, 0, PGSIZE);                          // zero headers

  /* one pipe per child to send mapped address */
  int p2c[MAX_CHILDREN][2];
  int child_pid[MAX_CHILDREN];

  for (int i = 0; i < MAX_CHILDREN; i++) {
    pipe(p2c[i]);
    int idx = i;
    int pid = fork();
    if (pid == 0) {                       /* ---------- child ---------- */
      close(p2c[idx][1]);                 // read only

      uint64 shared;
      read(p2c[idx][0], &shared, sizeof(shared));
      close(p2c[idx][0]);

      uint64 page_end = (shared & ~(PGSIZE - 1)) + PGSIZE;

      /* build message */
      char msg[MSG_LEN];
      char num[8];
      itoa(idx, num);
      strcpy(msg, "Hello from child ");
      strcpy(msg + strlen(msg), num);
      int len = strlen(msg);

      /* first header = first 4-byte boundary in page */
      uint8 *p = (uint8 *)(((shared + 3) & ~3));

      while ((uint64)p + sizeof(struct log_header) + len <= page_end) {
        struct log_header *h = (struct log_header *)p;
        uint32 packed = (idx << 16) | len;
        if (__sync_val_compare_and_swap((uint32 *)h, 0, packed) == 0) {
          memmove(p + sizeof(struct log_header), msg, len);
          break;
        }
        /* slot occupied: skip exactly that message */
        uint16 skip = h->msg_len;
        if (skip == 0) skip = 1;                     // corrupt -> move 1 byte
        p += sizeof(struct log_header) + skip;
        p  = (uint8 *)(((uint64)p + 3) & ~3);
      }
      exit(0);
    }
    /* ---------- parent after fork ---------- */
    close(p2c[i][0]);
    child_pid[i] = pid;
  }

  /* map buffer into each child, send address */
  for (int i = 0; i < MAX_CHILDREN; i++) {
    uint64 addr_in_child = map_shared_pages(logbuf, PGSIZE, child_pid[i]);
    write(p2c[i][1], &addr_in_child, sizeof(addr_in_child));
    close(p2c[i][1]);
  }

  /* wait for children */
  for (int i = 0; i < MAX_CHILDREN; i++) wait(0);

  /* ---------- parent scans & prints ---------- */
  uint64 page_end = base + PGSIZE;
  uint8 *p = (uint8 *)(((base + 3) & ~3));           // first aligned hdr
  while ((uint64)p + sizeof(struct log_header) < page_end) {
    struct log_header *h = (struct log_header *)p;
    uint16 len = h->msg_len;
    uint16 idx = h->child_idx;
    if (len && idx) {
      if ((uint64)p + sizeof(struct log_header) + len > page_end) break;
      char buf[MSG_LEN+1];
      memmove(buf, p + sizeof(struct log_header), len);
      buf[len] = 0;
      printf("Parent read from child %d: %s\n", idx, buf);
      p += sizeof(struct log_header) + len;
    } else {
      p += sizeof(struct log_header);
    }
    p = (uint8 *)(((uint64)p + 3) & ~3);
  }
  exit(0);
}
