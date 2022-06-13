#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "watchpoint.h"
#include "watchpointalloc.h"


void handler(void *addr, 
             void *old_val, 
             void *ucontext, 
             void *user_data)
{
    return;
}

void benchmark(size_t sizes, size_t writes) {
    if (sizes > writes)
        return;
    writes = writes / sizes;

    wpalloc_init();
    watchpoint_init();

    clock_t start, end;
    char *list = (char*)wpalloc(sizeof(char) * sizes);

    start = clock();

    for (size_t count = 0; count < writes; count++) {
        for (size_t i = 0; i < sizes; i++) {
            list[i] = 'a';
        }
    }

    end = clock();

    printf("      ((%lu, %f), ", end - start, (double)(end - start) / CLOCKS_PER_SEC);

    for (size_t i = 0; i < sizes; i++) {
        watchpoint_add(&(list[i]), &handler, NULL);
    }

    start = clock();

    for (size_t count = 0; count < writes; count++) {
        for (size_t i = 0; i < sizes; i++) {
            list[i] = 'a';
        }
    }

    end = clock();

    printf("(%lu, %f)),\n", end - start, (double)(end - start) / CLOCKS_PER_SEC);
    fflush(stdout);

    watchpoint_fini();
    wpalloc_fini();
}


int main(int argc, char *argv[])
{
    size_t benchmark_sizes[] = {1, pow(2, 4), pow(2, 8), pow(2, 12), pow(2, 16), pow(2, 20)};
    size_t benchmark_writes[] = {pow(2, 16), pow(2, 18), pow(2, 20), pow(2, 22)};
    int benchmark_runs = 5;

    size_t sizes, writes;
    printf("{\n");
    for (int i = 0; i < (sizeof(benchmark_sizes) / sizeof(size_t)); i++) {
        sizes = benchmark_sizes[i];
        printf("  %lu: {\n", sizes);
        for (int j = 0; j < (sizeof(benchmark_writes) / sizeof(size_t)); j++) {
            writes = benchmark_writes[j];
            printf("    %lu: {\n", writes);
            for (int k = 0; k < benchmark_runs; k++) {
                benchmark(sizes, writes);
            }
            printf("    },\n");
        }
        printf("  },\n");
    }
    printf("}\n");
    
    // for (size_t size = *benchmark_sizes; size != 0; size = *(&(size) + sizeof(size_t))) {
    //     printf("%li\n", size);
    // }

    // wpalloc_init();
    // watchpoint_init();

    // // int *x = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    // int *x = (int*)wpalloc(sizeof(int));
    // *x = 5;

    // int *y = (x + sizeof(int));
    // *y = 9;

    // printf("[main] Registering watchpoint...\n");
    // watchpoint_add(x, &handler, NULL);
    // printf("[main] Registered %p!\n", x);

    // for (int i = 0; i < 10; i++) {
    //     printf("[main] Increasing x by %i...\n", i);
    //     *x += i;
    //     printf("[main] x changed...\n");
    //     *y += 1;
    //     printf("[main] y changed...\n");
    //     printf("[main] x: %i  y: %i\n", *x, *y);
    // }

    // watchpoint_fini();
    // wpalloc_fini();

    return 0;
}
