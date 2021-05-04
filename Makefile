CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined

server: server.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

server.o: server.c
	$(CC) -c $(CFLAGS) server.c -lm -lpthread

clean:
	rm -f *.o server
