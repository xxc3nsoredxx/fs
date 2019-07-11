OBJS = main.o protocol.o server.o
CC = gcc
CFLAGS = -c -Wall -Wextra

all: fs

fs: $(OBJS)
	$(CC) $(OBJS) -o fs

main.o: main.c server.h
	$(CC) $(CFLAGS) main.c

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) protocol.c

server.o: server.c protocol.h server.h
	$(CC) $(CFLAGS) server.c
