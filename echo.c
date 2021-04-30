#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#define BACKLOG 5

int running = 1;

// the argument we will pass to the connection-handler threads
struct connection {
	struct sockaddr_storage addr;
	socklen_t addr_len;
	int fd;
};

int server(char *port);
void *echo(void *arg);

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	(void) server(argv[1]);
	return EXIT_SUCCESS;
}

void handler(int signal)
{
	running = 0;
}


int server(char *port)
{
	struct addrinfo hint, *info_list, *info;
	struct connection *con;
	int error, sfd;
	pthread_t tid;

	// initialize hints
	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;
		// setting AI_PASSIVE means that we want to create a listening socket

	// get socket and address info for listening port
	// - for a listening socket, give NULL as the host name (because the socket is on
	//   the local host)
	error = getaddrinfo(NULL, port, &hint, &info_list);
	if (error != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		return -1;
	}

	// attempt to create socket
	for (info = info_list; info != NULL; info = info->ai_next) {
		sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		
		// if we couldn't create the socket, try the next method
		if (sfd == -1) {
			continue;
		}

		// if we were able to create the socket, try to set it up for
		// incoming connections;
		// 
		// note that this requires two steps:
		// - bind associates the socket with the specified port on the local host
		// - listen sets up a queue for incoming connections and allows us to use accept
		if ((bind(sfd, info->ai_addr, info->ai_addrlen) == 0) &&
			(listen(sfd, BACKLOG) == 0)) {
			break;
		}

		// unable to set it up, so try the next method
		close(sfd);
	}

	if (info == NULL) {
		// we reached the end of result without successfuly binding a socket
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	freeaddrinfo(info_list);

	struct sigaction act;
	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGINT, &act, NULL);
	
	sigset_t mask;
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);


	// at this point sfd is bound and listening
	printf("Waiting for connection\n");
	while (running) {
		// create argument struct for child thread
		con = malloc(sizeof(struct connection));
		con->addr_len = sizeof(struct sockaddr_storage);
			// addr_len is a read/write parameter to accept
			// we set the initial value, saying how much space is available
			// after the call to accept, this field will contain the actual address length
		
		// wait for an incoming connection
		con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
			// we provide
			// sfd - the listening socket
			// &con->addr - a location to write the address of the remote host
			// &con->addr_len - a location to write the length of the address
			//
			// accept will block until a remote host tries to connect
			// it returns a new socket that can be used to communicate with the remote
			// host, and writes the address of the remote hist into the provided location
		
		// if we got back -1, it means something went wrong
		if (con->fd == -1) {
			perror("accept");
			continue;
		}
		
		// temporarily block SIGINT (child will inherit mask)
		error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
		if (error != 0) {
			fprintf(stderr, "sigmask: %s\n", strerror(error));
			abort();
		}

		// spin off a worker thread to handle the remote connection
		error = pthread_create(&tid, NULL, echo, con);

		// if we couldn't spin off the thread, clean up and wait for another connection
		if (error != 0) {
			fprintf(stderr, "Unable to create thread: %d\n", error);
			close(con->fd);
			free(con);
			continue;
		}

		// otherwise, detach the thread and wait for the next connection request
		pthread_detach(tid);

		// unblock SIGINT
		error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
		if (error != 0) {
			fprintf(stderr, "sigmask: %s\n", strerror(error));
			abort();
		}

	}

	puts("No longer listening.");
	pthread_detach(pthread_self());
	exit(EXIT_SUCCESS);
	pthread_exit(NULL);

	// never reach here
	return 0;
}

#define BUFSIZE 8

void *echo(void *arg)
{
	char host[100], port[10];
	struct connection *c = (struct connection *) arg;
	int error, nread;

	// find out the name and port of the remote host
	error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
		// we provide:
		// the address and its length
		// a buffer to write the host name, and its length
		// a buffer to write the port (as a string), and its length
		// flags, in this case saying that we want the port as a number, not a service name
	if (error != 0) {
		fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
		close(c->fd);
		return NULL;
	}

	printf("[%s:%s] connection\n", host, port);
	char request_code[4] = "";
	char request_length_str[10] = "";
	int request_length = 0;
	char key[100] = "";
	char value[100] = "";
	char curr_char;
	int i = 0;
	// int counter = 0;
	int is_code = 1; //request code not finished
	int is_length = 1; //request length not finished
	int is_key = 1; //key not finished
	int is_value = 1; //value not finished

	while ((nread = read(c->fd, &curr_char, 1)) > 0) {
		
		// build request code
		if (is_code == 1) {
			if (curr_char != '\n') {
				strncat(request_code, &curr_char, 1);
				continue;
			}
			is_code = 0;
			request_code[3] = '\0'; //string must end in terminator
			continue;
		}
		
		// build request length
		if (is_length == 1) {
			if (curr_char != '\n') {
				strncat(request_length_str, &curr_char, 1);
				continue;
			}
			is_length = 0;
			request_length_str[strlen(request_length_str)] = '\0';
			request_length = atoi(request_length_str);
			
			i = request_length;
			continue;		
		}

		// build the key
		if (is_key == 1) {
			i--;
			if (curr_char != '\n') {
				strncat(key, &curr_char, 1);
				continue;
			}
			is_key = 0;
			key[strlen(key)] = '\0';
			continue;
		}

		if (strcmp("SET", request_code) == 0) {
			// build the value
			if (is_value == 1) {
				i--;
				if (curr_char != '\n') {
					strncat(value, &curr_char, 1);
					continue;
				}
				// is_value = 0;
				value[strlen(value)] = '\0';
				if (i != 0) {
					write(c->fd, "ERR\nLEN\n", 9);
					exit(EXIT_FAILURE);
				}
				is_code = 1;
				is_length = 1;
				is_key = 1;
				is_value = 1;

				char* value_length = malloc(3 * sizeof(char));
				sprintf(value_length, "%ld", strlen(value)+1);
				int size = 7 + strlen(value_length) + strlen(value);
				char* response = malloc(size * sizeof(char));
				strcpy(response, "OKG\n");	
				strncat(response, value_length, strlen(value_length));
				strncat(response, "\n", 1);
				strncat(response, value, strlen(value));
				strncat(response, "\n", 1);

				write(c->fd, response, strlen(response));
				// printf("%s", response);

				free(value_length);
				free(response);
				continue;
			}
		}
		
















		// if (curr_char == '\n')
		// {
		// 	if (is_key == 0 || (strcmp(request_code, "DEL") == 0) || (strcmp(request_code, "GET") == 0))
		// 	{
		// 		// we've reached the end of the request
		// 		// counter = 0;

		// 		is_key = 1;

		// 		// send the request away!
		// 		printf("request code: %s\n", request_code);
		// 		printf("request length: %d\n", request_length);
		// 		printf("key: %s\n", key);
		// 		printf("value: %s\n", value);

		// 		//clear key and value
		// 		memset(key, '\0', sizeof key);
		// 		memset(value, '\0', sizeof value);
		// 		break;
		// 	}
		// 	else
		// 	{
		// 		is_key = 0;
		// 	}
		// 	i++;
		// }
		// if (is_key == 1)
		// {
		// 	strncat(key, &buf[i], 1);
		// }
		// else
		// {
		// 	strncat(value, &buf[i], 1);
		// }
		
		// //reached the end of buffer
		// if (i >= BUFSIZ - 1) {
		// 	i = 0;
		// 	newRequest = 1;
		// 	continue;
		// }
		
		/*
		if (nread < BUFSIZE && request_length == 0)
		{
			//err
			printf("error- invalid argument from client\n");
			close(c->fd);
			free(c);
			return NULL;
		}
		*/

		//printf("%s\n", buf);
	}

	printf("[%s:%s] got EOF\n", host, port);

	close(c->fd);
	free(c);
	exit(EXIT_SUCCESS);
	return NULL;
}