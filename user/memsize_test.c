#include "kernel/types.h"
 #include "user/user.h"
 
 int main(void) {
     printf("Initial memory size: %d bytes\n", memsize());
 
     void *p = malloc(20 * 1024);  // Allocate 20KB
     if (p == 0) {
         printf("malloc failed\n");
         exit(1);
     }
 
     printf("After malloc 20KB: %d bytes\n", memsize());
 
     free(p);
 
     printf("After free: %d bytes\n", memsize());
 
     exit(0);
 }