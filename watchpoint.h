int watchpoint_init();
int watchpoint_fini();
/* The handler function takes the address of the triggered
 * watchpoint, the old value, new value, and user_data. */
int watchpoint_add(void *addr, int (*handler)(void*, void*, void*, void*), void *user_data);
int watchpoint_rem(void *addr);
