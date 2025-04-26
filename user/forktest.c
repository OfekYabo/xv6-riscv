// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N  1000

void
print(const char *s)
{
  write(1, s, strlen(s));
}

void
forktest(void)
{
  int n, pid;
  char exit_msg[32]; // Buffer for exit messages

  print("fork test\n");

  for(n = 0; n < N; n++) {
    pid = fork();
    if(pid < 0)
      break;
    if(pid == 0)
      exit(0, "Child exited");
  }

  if(n == N) {
    print("fork claimed to work N times!\n");
    exit(1, "Fork limit exceeded");
  }

  for(; n > 0; n--) {
    if(wait(0, exit_msg) < 0) {
      print("wait stopped early\n");
      exit(1, "Wait stopped early");
    }
  }

  if(wait(0, exit_msg) != -1) {
    print("wait got too many\n");
    exit(1, "Too many waits");
  }

  print("fork test OK\n");
}

int
main(void)
{
  forktest();
  exit(0, "Fork test completed");
}
