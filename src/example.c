#define _GNU_SOURCE  // Required for REG_RIP
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include "watchpoint.h"
#include "watchpointalloc.h"

typedef struct handler_data {
    void *rips[8];
    int i;
} handler_data;


void handler(void *addr, void *old_val, void *ucontext, void *user_data)
{
    // https://www.linuxjournal.com/article/6391
    ucontext_t *context = (ucontext_t *)ucontext;
    handler_data *data = (handler_data *)user_data;
    data->rips[data->i++] = (void *)context->uc_mcontext.gregs[REG_RIP];
    // void *trace_buff[2];
    // trace_buff[1] = (void *)context->uc_mcontext.gregs[REG_RIP];
    // char **trace_names = backtrace_symbols(trace_buff, 2);
    // printf("[handler] Change occurred at %s\n", trace_names[1]);
    printf("[handler] The old value was %i, now it is %i.\n",
           *(int*)&old_val, *(int*)addr);
}

void foo(int *x)
{
    *x = 1;
}

void bar(int *x)
{
    *x = 2;
}

void baz(int *x)
{
    *x = 3;
}

int main(int argc, char *argv[])
{
    wpalloc_init();
    watchpoint_init();

    handler_data *data = malloc(sizeof(handler_data));
    data->i = 0;

    // int *x = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *x = (int*)wpalloc(sizeof(int));
    *x = 5;

    int *y = (x + sizeof(int));
    *y = 9;

    printf("[main] Registering watchpoint...\n");
    watchpoint_add(x, &handler, data);
    printf("[main] Registered %p!\n", x);

    /* Change x in different functions */
    *x = 0;
    foo(x);
    bar(x);
    baz(x);
    *x = 4;

    /* Change x and y (which is unwatched) */
    for (int i = 0; i < 4; i++) {
        printf("[main] Increasing x by %i...\n", i);
        *x += i;
        printf("[main] x changed...\n");
        *y += 1;
        printf("[main] y changed...\n");
        printf("[main] x: %i  y: %i\n", *x, *y);
    }

    watchpoint_fini();
    wpalloc_fini();

    char **trace_names;
    void *trace_buff[1];
    for (int i = 0; i < data->i; i++) {
        printf("%p\n", data->rips[i]);
        trace_buff[0] = data->rips[i];
        trace_names = backtrace_symbols(trace_buff, 1);
        printf("%s\n", trace_names[0]);
    }

    return 0;
}
