#define _GNU_SOURCE  // Required for REG_RIP
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include "watchpoint.h"
#include "watchpointalloc.h"


void handler(void *addr, void *old_val, void *ucontext, void *user_data)
{
    // https://www.linuxjournal.com/article/6391
    ucontext_t *context = (ucontext_t *)ucontext;
    void *trace_buff[2];
    trace_buff[1] = (void *)context->uc_mcontext.gregs[REG_RIP];
    char **trace_names = backtrace_symbols(trace_buff, 2);
    printf("[handler] Change occurred at %s\n", trace_names[1]);
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

    int *y = (x + sizeof(int));
    *y = 9;

    printf("[main] Registering watchpoint...\n");
    watchpoint_add(x, &handler, NULL);
    printf("[main] Registered %p!\n", x);

    for (int i = 0; i < 10; i++) {
        printf("[main] Increasing x by %i...\n", i);
        *x += i;
        printf("[main] x changed...\n");
        *y += 1;
        printf("[main] y changed...\n");
        printf("[main] x: %i  y: %i\n", *x, *y);
    }

    watchpoint_fini();
    wpalloc_fini();

    return 0;
}
