#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "watchpoint.h"
#include <ucontext.h>
#include <signal.h>
#include "watchpointalloc.h"

void handler(void *addr, void *old_val, void *ucontext void *user_data)
{
    ucontext_t *context = ucontext;
    printf("[handler] Function: %x\n", context->uc_stack.ss_sp);
    printf("[handler] The old value was %i, now it is %i.\n",
           *(int*)old_val, *(int*)addr);
}

int main(int argc, char *argv[])
{
    wpalloc_init();
    watchpoint_init();

    // int *x = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *x = (int*)wpalloc(sizeof(int));
    *x = 5;

    printf("[main] Registering watchpoint...\n");
    watchpoint_add(x, &handler, NULL);
    printf("[main] Registered %p!\n", x);

    for (int i = 0; i < 10; i++) {
        printf("[main] Increasing x by %i...\n", i);
        *x += i;
        printf("[main] %i\n", *x);
    }

    watchpoint_fini();
    wpalloc_fini();

    return 0;
}
