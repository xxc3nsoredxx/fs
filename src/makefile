OBJS = client.o main.o protocol.o server.o
CC = gcc
CFLAGS = -c -Wall -Wextra `libgcrypt-config --cflags`
LFLAGS = `libgcrypt-config --libs`

all: fs

fs: $(OBJS)
	$(CC) $(OBJS) -o fs $(LFLAGS)

client.o: client.c client.h protocol.h
	$(CC) $(CFLAGS) client.c

main.o: main.c client.h server.h
	$(CC) $(CFLAGS) main.c

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) protocol.c

server.o: server.c protocol.h server.h
	$(CC) $(CFLAGS) server.c
