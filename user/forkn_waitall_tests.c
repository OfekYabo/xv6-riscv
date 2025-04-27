#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_CHILDREN 4

int main(void) {
    int n = MAX_CHILDREN;
    int pids[MAX_CHILDREN];

    // Test forkn with valid input
    printf("Testing forkn with n = %d\n", n);
    int ret = forkn(n, pids);
    if (ret < 0) {
        printf("forkn failed\n");
        exit(1, "");
    }

    if (ret == 0) {
        // Parent process
        printf("Parent: forked children PIDs: ");
        for (int i = 0; i < n; i++)
            printf("%d ", pids[i]);
        printf("\n");

        int statuses[64];  // NPROC max
        int num_waited;

        // Wait for all children
        if (waitall(&num_waited, statuses) < 0) {
            printf("waitall failed\n");
            exit(1, "");
        }

        printf("Parent: waited for %d children\n", num_waited);
        for (int i = 0; i < num_waited; i++)
            printf("Child exit status: %d\n", statuses[i]);

        printf("Test completed successfully\n");
        exit(0, "parent done");
    } else {
        // Child process
        printf("Child %d: PID %d exiting\n", ret, getpid());
        exit(ret * 10, "");  // Exit with a status to check
    }

    return 0;
}