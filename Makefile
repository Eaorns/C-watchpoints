CFLAGS = -g -Wall
CC = gcc
OBJS = watchpoint.o watchpointalloc.o
OBJP = $(OBJS:%.o=src/%.o)

.o:
	$(CC) $(CFLAGS) -c $<

example: ${OBJP}
	${CC} src/example.c ${CFLAGS} -o $@ ${OBJP}

clean:
	rm src/*.o example

all: example
