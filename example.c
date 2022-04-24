#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "watchpoint.h"
#include "watchpointalloc.h"

int main(int argc, char *argv[])
{
    wpa_init();
    watchpoint_init();

    // int *x = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *x = (int*)wpalloc(sizeof(int));
    *x = 5;

    printf("[main] Registering watchpoint...\n");
    watchpoint_add(x, NULL, NULL);
    printf("[main] Registered %p!\n", x);

    for (int i = 0; i < 10; i++) {
        printf("[main] Increasing x by %i...\n", i);
        *x += i;
        printf("[main] %i\n", *x);
    }

    watchpoint_fini();

    return 0;
}
