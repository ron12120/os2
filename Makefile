CC=gcc
CFLAGS=-Wall -lm -lcrypto

all: server client

server: server.c
	$(CC) -o server server.c $(CFLAGS)

client: client.c
	$(CC) -o client client.c $(CFLAGS)
clean:
	rm -f server client
