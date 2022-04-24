# C-watchpoints
Allows adding breakpoints to variables, with custom handlers.

## How it works
To detect changes to a value on a memory address, the page in which the address
is located is marked as read-only (see [mprotect(2)](https://man7.org/linux/man-pages/man2/mprotect.2.html)). Whenever a value is written to
the address, it will generate a `SIGSEGV`, which is catched. If the address that 
generated the segmentation fault is an address that is being watched, the processor
is put into single-step mode, the read-only protection is temporarily removed, and 
the execution continued. If a segfault is generated again, it is a genuine 
segmentation fault and the default handler is restored. Normally, a `SIGSTEP` should 
be generated after the write was completed successfully. This is also catched, after
which the processor is taken out of single-step mode, and the read-only protection
is restored. Finally, the by the user provided watchpoint handler is called. 

## Usage
Always start with `watchpoint_init()` and end with `watchpoint_fini()`.

To register a watchpoint, simply call 
```c
int watchpoint_add(void *addr, void (*handler)(void*, long, void*), void *user_data)
```
The function takes the address of the variable to place the watchpoint on, as
well as a function pointer to a function to call when the watchpoint is triggered,
and a pointer to user data that is passed to the given handler function.

The handler function should have the signature
```c
void handler(void *addr, long old_data, void *user_data)
```
When called by the watchpoint handler, it is given the address of the variable
that is changed, its old value as a `long`, and the pointer to the user data given
to the `watchpoint_add()`-function. The new value is not included as this can
(obviously) be read from the given address. **Please have a look at signal-safe
functions (see [Limitations & general advice](#limitations--general-advice)) to use inside the handler.**

Watchpoints may also be removed using
```c
int watchpoint_rem(void *addr)
```

### WPalloc
Because of limitations of the normal `malloc()`, the provided `wpalloc()` should
be used to allocate all memory on which watchpoints will be placed. It should work
as a drop-in replacement for `malloc()`. Alternatively, you can allocate memory 
using [mmap(2)](https://www.man7.org/linux/man-pages/man2/mmap.2.html), which is what `wpalloc()` does internally.

To free memory, `wpfree()` can be used. Again, it should be a drop-in replacement
for normal free on memory allocated with `wpalloc()`.

The implementation of `wpalloc()` is very basic and inefficient, seen from a 
memory management perspective. While it is usable, it may be desirable to replace 
it with something more efficient.

## Limitations & general advice
Because of the overhead, it is advised to only allocate memory which will be watched
or will not be written to inside the memory pages with watched values as much as
possible.

Since a signal handler is used, only signal-safe functions should be used inside the
handler. **`printf()` is not a signal-safe function!** See [signal-safety(7)](https://man7.org/linux/man-pages/man7/signal-safety.7.html). Using other 
functions (including `printf`) may work fine, however, it may also lead to weird and
difficulut-to-trace bugs.
While `malloc()` is normally also not a signal-safe function, I believe it should work 
fine in most as long as it allocates memory outside of the protected pages. **But this
is still bad practice and not really advised!**

## Future plans & features
Suggestions are always welcome!

 - Investigate a watchpoint handler that uses 'wrapper'-structs instead of lists,
   but can only work on single values. This might be more efficient. This may
   work a bit more like `getter()`- and `setter()`-functions in object-oriented 
   languages.
 - Implement a better memory manager to replace the current WPalloc.