/*
- program takes one argument, an integer identifying which port it will use to listen on (between 5000 and 65536)
- opens and binds a listening socket on the specified port
- waits for incoming connection requests
	- For each connection:
		- create a thread to handle communication with that client
		- thread terminates once the connection is closed
- maintains the key-value data in a data structure shared by all threads
*/