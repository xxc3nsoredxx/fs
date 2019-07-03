OBJS = main.o
CC = gcc
CFLAGS = -c -Wall -Wextra

all: fs

fs: $(OBJS)
	$(CC) $(OBJS) -o fs

main.o: main.c
	$(CC) $(CFLAGS) main.c
