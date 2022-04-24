CFLAGS = -g -Wall
CC = gcc
OBJS = watchpoint.o watchpointalloc.o

.o:
	$(CC) $(CFLAGS) -c $<

example: ${OBJS}
	${CC} example.c ${CFLAGS} -o $@ ${OBJS}

clean:
	rm *.o example

all: example
