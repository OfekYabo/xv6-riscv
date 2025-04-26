#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    exit(0, "Goodbye World xv6");
    return 0; // This line will never be reached
}