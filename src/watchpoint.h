int watchpoint_init();
int watchpoint_fini();
/* The handler function takes the address of the triggered
 * watchpoint, the old value, a pointer to the ucontext,
 * and user_data. */
int watchpoint_add(void *addr, void (*handler)(void*, void*, void*, void*), void *user_data);
int watchpoint_rem(void *addr);
