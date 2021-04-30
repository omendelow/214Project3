CC = gcc
CFLAGS = -g -std=c99 -Wvla -Wall -fsanitize=address,undefined

client: client.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

client.o: client.c
	$(CC) -c $(CFLAGS) client.c -lm -lpthread

echo: echo.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

echo.o: echo.c
	$(CC) -c $(CFLAGS) echo.c -lm -lpthread

server: server.o
	$(CC) $(CFLAGS) -o $@ $^ -lm -lpthread

server.o: server.c
	$(CC) -c $(CFLAGS) server.c -lm -lpthread

clean:
	rm -f *.o client echo server
