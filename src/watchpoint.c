#define _GNU_SOURCE // Required for REG_RIP which is otherwise not recognised
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ucontext.h>
#include <sys/mman.h>
#include "watchpoint.h"

extern int errno;

// #define DO_WP_DEBUG
#ifdef DO_WP_DEBUG
#define WP_DEBUG printf
#else
#define WP_DEBUG(...) while (0) {}
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#endif

#define PAGE_MASK           (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr)    ((void*)(((long)addr) & PAGE_MASK))

#define PAGE_TABLE_SIZE 4096
#define PAGE_TABLE_BASE(addr) (((long)addr >> 12) % PAGE_TABLE_SIZE)

#define WP_TABLE_SIZE 4096
#define WP_TABLE_BASE(addr) ((long)addr % WP_TABLE_SIZE)

/* Set the trapflag value based on the target architecture */
// https://en.wikipedia.org/wiki/FLAGS_register
#define TRAPFLAG_X86 0x0100
#define TRAPFLAG TRAPFLAG_X86

typedef void (*wp_handler)(void*, long, void*);

/* Struct to keep track of which pages contain breakpoints,
 * Also stores how many there are in the page. */
typedef struct wpage {
    void *addr;
    int counter;
    struct wpage *next;
} wp_page;

/* Struct to store a watchpoint.
 * Keeps track of the address where the watchpoint is placed,
 * as well as the accompanying handler function and provided
 * user data. */
typedef struct waddr {
    void *addr;
    wp_handler handler;
    void *data;
    struct waddr *next;
} wp_addr;

int num_watchpoints;
wp_page **page_table;
wp_addr **wp_table;
void *curr_segv_addr;
long prev_val;

/**
 * Retrieve a wp_page entry from the page table,
 * given an address in the page.
 */
wp_page *wp_page_get(void *addr)
{
    wp_page *entry = page_table[PAGE_TABLE_BASE(addr)];

    while (entry != NULL) {
        if (entry->addr == addr)
            return entry;
        entry = entry->next;
    }
    return NULL;
}

void wp_page_inc(void *addr)
{
    WP_DEBUG("[watchpoint handler] Protecting address %p ", addr);
    addr = PAGE_ALIGN(addr);
    WP_DEBUG("in page %p\n", addr);
    wp_page *page = wp_page_get(addr);

    if (page != NULL) {
        page->counter++;
    } else {
        page = malloc(sizeof(wp_page));
        page->addr = addr;
        page->counter = 1;
        page->next = page_table[PAGE_TABLE_BASE(addr)];
        page_table[PAGE_TABLE_BASE(addr)] = page;

        WP_DEBUG("[watchpoint handler] Calling mprotect...\n");
        if (mprotect(addr, PAGE_SIZE, PROT_READ) != 0) {
            WP_DEBUG("[watchpoint handler] Error calling mprotect: ");
            switch (errno) {
                case EACCES:
                    WP_DEBUG("EACCES\n");
                    break;
                case EINVAL:
                    WP_DEBUG("EINVAL\n");
                    break;
                case ENOMEM:
                    WP_DEBUG("ENOMEM\n");
                    break;
                default:
                    WP_DEBUG("UNKNOWN ERROR\n");
            }
            return;
        }
    }
    WP_DEBUG("[watchpoint handler] Address protected!\n");
}

void wp_page_dec(void *addr)
{
    addr = PAGE_ALIGN(addr);
    wp_page *page = wp_page_get(addr);
    if (page->addr != addr) {
        /* This should never happen! */
        return;
    }
    page->counter--;

    if (page->counter == 0) {
        if (page_table[PAGE_TABLE_BASE(addr)] == page) {
            page_table[PAGE_TABLE_BASE(addr)] = page->next;
        } else {
            wp_page *prev = page_table[PAGE_TABLE_BASE(addr)];
            while (prev->next != page) {
                prev = prev->next;
            }
            prev->next = page->next;
        }

        /* Remove protection from page */
        mprotect(addr, PAGE_SIZE, PROT_READ | PROT_WRITE);
        free(page);
    }
}

wp_addr *wp_addr_get(void *addr)
{
    wp_addr *entry = wp_table[WP_TABLE_BASE(addr)];

    while (entry != NULL) {
        if (entry->addr == addr)
            return entry;
        entry = entry->next;
    }
    return NULL;
}

wp_addr *wp_addr_add(void *addr, wp_handler handler, void *user_data)
{
    wp_addr *entry = wp_addr_get(addr);
    if (entry != NULL) {
        entry->handler = handler;
        return entry;
    }

    entry = malloc(sizeof(wp_addr));

    entry->addr = addr;
    entry->handler = handler;
    entry->data = user_data;
    entry->next = wp_table[WP_TABLE_BASE(addr)];

    wp_table[WP_TABLE_BASE(addr)] = entry;
    wp_page_inc(addr);

    num_watchpoints++;
    return entry;
}

int wp_addr_rem(void *addr)
{
    wp_addr *prev = NULL, *curr = wp_table[WP_TABLE_BASE(addr)];

    while (curr != NULL) {
        if (curr->addr == addr)
            break;
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        return -1;
    }

    if (prev == NULL) {
        // TODO what about everything before?
            wp_table[WP_TABLE_BASE(addr)] = curr->next;
        } else {
        prev->next = curr->next;
    }
    free(curr);
    num_watchpoints--;

    wp_page_dec(addr);

    return 0;
}

void watchpoint_sigsegv(int signo, siginfo_t *info, void *vcontext)
{
    WP_DEBUG("[watchpoint handler] SIGSEGV at %p\n", info->si_addr);
    // TODO also check if SIGSEGV type is as expected & possibly if page is in use
    //      (which may be more efficient than checking the address)
    if (info->si_addr == NULL || curr_segv_addr != NULL || wp_addr_get(info->si_addr) == NULL) {
        WP_DEBUG("[watchpoint handler] Non-intentional SIGSEGV! Restoring default handler...\n");

        /* Restore default sighandler */
        struct sigaction actsegv = { 0 };
        actsegv.sa_handler = SIG_DFL;
        sigaction(SIGSEGV, &actsegv, NULL);

        return;
    }
    curr_segv_addr = info->si_addr;

    WP_DEBUG("[watchpoint handler] Old value: %i\n", *(int*)curr_segv_addr);
    prev_val = *(long*)curr_segv_addr;

    /* Enable single-step flag */
    ucontext_t *context = vcontext;
    context->uc_mcontext.gregs[REG_EFL] |= TRAPFLAG_X86;

    wp_page *page = wp_page_get(PAGE_ALIGN(curr_segv_addr));
    mprotect(page->addr, PAGE_SIZE, PROT_READ | PROT_WRITE);

    return;
}

void watchpoint_sigtrap(int signo, siginfo_t *info, void *vcontext)
{
    WP_DEBUG("[watchpoint handler] New value: %i\n", *(int*)curr_segv_addr);

    /* Disable single-step flag */
    ucontext_t *context = vcontext;
    context->uc_mcontext.gregs[REG_EFL] ^= TRAPFLAG_X86;

    wp_page *page = wp_page_get(PAGE_ALIGN(curr_segv_addr));
    mprotect(page->addr, PAGE_SIZE, PROT_READ);

    wp_addr *addr = wp_addr_get(curr_segv_addr);
    if (addr->handler != NULL)
        (addr->handler)(curr_segv_addr, prev_val, addr->data);

    curr_segv_addr = NULL;
    return;
}

int watchpoint_init()
{
    WP_DEBUG("[watchpoint handler] Starting watchpoint init...\n");
    num_watchpoints = 0;
    page_table = malloc(sizeof(void*) * PAGE_TABLE_SIZE);
    wp_table = malloc(sizeof(void*) * WP_TABLE_SIZE);
    curr_segv_addr = NULL;

    WP_DEBUG("[watchpoint handler] Page size: %li\n", PAGE_SIZE);

    /* Register SIGSEGV handler */
    struct sigaction actsegv = { 0 };
    actsegv.sa_flags = SA_SIGINFO;
    actsegv.sa_sigaction = &watchpoint_sigsegv;
    sigaction(SIGSEGV, &actsegv, NULL);

    /* Register SIGTRAP handler */
    struct sigaction acttrap = { 0 };
    acttrap.sa_flags = SA_SIGINFO;
    acttrap.sa_sigaction = &watchpoint_sigtrap;
    sigaction(SIGTRAP, &acttrap, NULL);

    WP_DEBUG("[watchpoint handler] Watchpoint init done!\n");
    return 0;
}

int watchpoint_fini()
{
    wp_addr *curr, *tmp;
    for (int i = 0; i < WP_TABLE_SIZE; i++) {
        curr = wp_table[WP_TABLE_BASE(i)];
        while (curr != NULL) {
            tmp = curr->next;
            wp_page_dec(curr->addr);
            free(curr);
            curr = tmp;
        }
    }
    free(wp_table);

    /* Restore default sighandler */
    struct sigaction actsegv = { 0 };
    actsegv.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &actsegv, NULL);

    return 0;
}

int watchpoint_add(void *addr, wp_handler handler, void *data)
{
    return (wp_addr_add(addr, handler, data) != NULL);
}

int watchpoint_rem(void *addr)
{
    return wp_addr_rem(addr);
}
