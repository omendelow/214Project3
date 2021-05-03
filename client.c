#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>

#define BUFSIZE 128

int main(int argc, char **argv)
{
	struct addrinfo hints, *info_list, *info;
	int error;
	int sock;
	int bytes;
	char buf[BUFSIZE+1];
	
	if (argc < 4) {
		printf("Usage: %s [host] [port] [message(s)...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// we need to provide some additional information to getaddrinfo using hints
	// we don't know how big hints is, so we use memset to zero out all the fields
	memset(&hints, 0, sizeof(hints));
	
	// indicate that we want any kind of address
	// in practice, this means we are fine with IPv4 and IPv6 addresses
	hints.ai_family = AF_UNSPEC;
	
	// we want a socket with read/write streams, rather than datagrams
	hints.ai_socktype = SOCK_STREAM;

	// get a list of all possible ways to connect to the host
	// argv[1] - the remote host
	// argv[2] - the service (by name, or a number given as a decimal string)
	// hints   - our additional requirements
	// info_list - the list of results

	error = getaddrinfo(argv[1], argv[2], &hints, &info_list);
	if (error) {
		fprintf(stderr, "%s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	
	// try each of the possible connection methods until we succeed
	for (info = info_list; info != NULL; info = info->ai_next) {
		// attempt to create the socket
		sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		
		// if we somehow failed, try the next method
		if (sock < 0) continue;
		
		// try to connect to the remote host using the socket
		if (connect(sock, info->ai_addr, info->ai_addrlen) == 0) {
			// we succeeded, so break out of the loop
			break;
		}

		// we weren't able to connect; close the socket and try the next method		
		close(sock);
	}
	
	// if we exited the loop without opening a socket and connecting, halt
	if (info == NULL) {
		fprintf(stderr, "Could not connect to %s:%s\n", argv[1], argv[2]);
		exit(EXIT_FAILURE);
	}
	
	// now that we have connected, we don't need the addressinfo list, so free it
	freeaddrinfo(info_list);


	char* message = "SET\n11\nday\nSunday\n";
	write(sock,message,strlen(message));

	write(sock, "GET\n6\nh", 7);
	sleep(1);
	write(sock, "ello\n", 5);
	
	write(sock, "GET\n4", 6);
	sleep(1);
	write(sock, "\nday\n",6);
	// sleep(1);
	// write(sock, "SET\n11\nday\nSunday\nGET\n6\na\nb c\n", 31);

	// write(sock, "SET\n6\na\nb c\n", 13);

	// write(sock, "GET\n4\nday\n", 11);

	while ((bytes = read(sock, buf, BUFSIZE)) > 0) {
		buf[bytes] = '\0';
		printf("%s", buf);
		// printf("Got %d bytes: |%s|\n", bytes, buf);
	}

	
	// close the socket
	close(sock);

	return EXIT_SUCCESS;	
}
