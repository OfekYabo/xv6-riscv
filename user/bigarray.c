#include "kernel/types.h"
#include "user/user.h"

#define ARRAY_SIZE (1 << 16) //TODO: return to this
// #define ARRAY_SIZE (1 << 12) //TODO: for testing
#define NUM_CHILDREN 4

int main(void) {
    int pids[NUM_CHILDREN];
    int statuses[NUM_CHILDREN];
    int num_collected;
    int *array = malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        printf("Failed to allocate memory for array\n");
        exit(1, "malloc failed");
    }


    //TODO: Debugging prints
    printf("before init, array=%p, pids=%p, statuses=%p\n", array, pids, statuses);
    // Initialize the array
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = i;
    }

    //TODO: Debugging prints
    printf("bigarray: pids=%p, statuses=%p, NUM_CHILDREN=%d\n", pids, statuses, NUM_CHILDREN);

    // Fork child processes
    int ret = forkn(NUM_CHILDREN, pids);
    if (ret < 0) {
        printf("Error: forkn failed\n");
        exit(1, "Error: forkn failed\n");
    }

    //TODO: Debugging prints
    printf("bigarray: forked children PIDs: ");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        printf("%d ", pids[i]);
    }
    printf("\n");

    // Child processes
    if (ret > 0) {
        // Child process logic
        int idx = ret - 1;  // Convert to 0-based index (1..n -> 0..n-1)
        int start = idx * (ARRAY_SIZE / NUM_CHILDREN);
        int end = start + (ARRAY_SIZE / NUM_CHILDREN);
        int partial_sum = 0;

        for (int j = start; j < end; j++) {
            partial_sum += array[j];
        }

        printf("Child %d partial sum: %d\n", idx + 1, partial_sum);
        exit(partial_sum, "");
    }

    // Parent process
    if (waitall(&num_collected, statuses) < 0) {
        printf("waitall failed\n");
        free(array);
        exit(1, "waitall failed");
    }

    //TODO: Debugging prints
    printf("bigarray: waited for %d children\n", num_collected);
    for (int i = 0; i < num_collected; i++) {
        //TODO: Debugging prints
        printf("bigarray: Child %d exit status: %d\n", i, statuses[i]);
    }

    // Verify the number of children
    if (num_collected != NUM_CHILDREN) {
        printf("waitall returned incorrect number of children\n");
        exit(1, "waitall mismatch");
    }

    // Calculate the total sum
    int total_sum = 0;
    for (int i = 0; i < num_collected; i++) {
        total_sum += statuses[i];
    }

    printf("Total sum: %d\n", total_sum);
    free(array);
    exit(0, "Calculation completed");
}