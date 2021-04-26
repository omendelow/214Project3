/*
- program initiates and terminates connection to the server
- once a connection is open
	- client will send zero or more requests to the server
	- for each request
		- server will take some action and respond
		- after receiving a response, client may
			- send another request
			- close the connection
*/