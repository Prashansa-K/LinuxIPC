// Standard header files
#include <stdio.h>
#include <stdlib.h> // For exit() calls
#include <string.h> // For memset()


// Specific to socket programming
#include <sys/socket.h> // Contains definitions for sockaddr struct and macros for SOCK_STREAM, SOCK_DGRAM, etc. -- methods for communication: streaming, datagrams, etc.
#include <sys/un.h> // Contains definition for sockaddr_un structure; sockaddr_un stores addresses for UNIX domain sockets
#include <unistd.h>

#define SOCKET_NAME "/tmp/UnixDomainSocket"
#define BUFFER_SIZE 128

// Server Program to receive numbers from client and send back the sum
int main (int argc, char *argv[]) {
	  struct sockaddr_un name;	

	/*
		Structure is as such:
			struct sockaddr_un {
				sa_family_t sun_family; // Protocol family used for communication over the socket (See socket() call desc.)
				char sun_path; // Name of the socket
			}
	*/

	// File descriptors / sockets are actually +ve integer values. Thus, using int to refer them.
	int connection_socket; // Master file descriptor - mother socket
	int data_socket; // This will hold the client socket to recieve data
	int client_data;
	int server_result;
	int ret; // Temp variable to check return values

	char buffer[BUFFER_SIZE];

	// Unlinking any existing socket by the same name
	// Useful when the previous program ended abruptly and there's socket already present by the same name on the same port
	// Prevents "Address already in use error"
	unlink(SOCKET_NAME);

	// Creating the master socket.
	// Using stream based communication - that means data flowing in/out of the socket will be in the form of byte stream
	connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	/*
		The above system call creates an endpoint for communication and returns a file descriptor referring to the endpoint.
		int socket(int domain, int type, int protocol) - provided by socket.h file
			domain = protocol family used for communication as defined in socket.h
				AF_UNIX implies local communication over the unix domain socket. AF_LOCAL is a synonym.

			type = indicates communication semantics.
				SOCK_STREAM implies connection-oriented, reliable, sequenced, 2-way byte stream ~ TCP
			 	SOCK_DGRAM implies a connectionless, unreliable messages of fixed length in form of datagrams ~ UDP
			
			protocol = For a particular socket type from a particular family (domain), usually a single protocol is supported.
				Protocol numbers provided here refer to the supported protocol. To refer what each number stands for, use
				/etc/protocols.
				0 implies IP protocol.
	*/

	if(connection_socket == -1) {
		perror("Master socket could not be created.");
		exit(EXIT_FAILURE);
	}	

	printf("Master socket is created...\n");

	// Initialise the master socket
	
	// Filling the memory block with 0 to avoid previously filled garbage values to corrupt the program.
	memset(&name, 0, sizeof(struct sockaddr_un));

	// Adding socket specifics to structure variable
	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

//	int reuse = 1;

	// kill "Address already in use" error message
//	if (setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
  //  		perror("setsockopt");
    //		exit(EXIT_FAILURE);
//	}

	// Bind the socket to socket name
	 ret = bind(connection_socket, (const struct sockaddr *) &name,
            sizeof(struct sockaddr_un));
	// ret = bind(connection_socket, (const struct sockadrr*)&name, sizeof(name));
	/*
 		When a socket is created with socket(), it exists in a namespace (address family) but has no address assigned to it.  
		
		int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

		bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd.  
		addrlen specifies the size, in bytes, of the address structure pointed to by addr.
       
		Traditionally, this operation is called “assigning a name to a socket” 
	*/


	if(ret == -1) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}	
	
	printf("bind() succeeded ...\n");

	// Start listening

	ret = listen(connection_socket, 20);
	/*
		int listen(int sockfd, int backlog);
		
		listen() marks the socket as a passive socket i.e, as a socket that will be used to accept incoming connection requests 
		using accept().

       		The sockfd argument is a file descriptor that refers to a socket of type SOCK_STREAM or SOCK_SEQPACKET.
		listen could be only for connection-oriented socket types, just like accept().

       		The backlog argument defines the maximum length to which the queue of pending connections for sockfd may grow.

		If a connection request arrives when the queue is full, the client may receive ECONNREFUSED error or, 
		if the underlying protocol supports retransmission, the request may be ignored so that a later reattempt at connection succeeds.
	*/

	if(ret == -1) {
		perror("listen() failed");
		exit(EXIT_FAILURE);
	}

	// A server process is supposed to run always
	// That's why the infinite loop, accepting client connections

	while(1) {
		printf("Waiting for a client... Calling accept() ...\n");
	
		data_socket = accept(connection_socket, NULL, NULL);
		/*
			accept() is a blocking system call i.e. the system waits till a client tries to connect with the server.
			Above happens only when the socket is not marked as nonblocking (default mode).	
			
			int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);

			The accept() system call is used with connection-based socket types (SOCK_STREAM, SOCK_SEQPACKET).  
			It extracts the first connection request on the queue of pending connections for the listening socket, sockfd, 
			creates a new connected socket, and returns a new file descriptor referring to that socket.
			The original connection socket sockfd is unaffected by this call.

			If the socket is marked nonblocking and no pending connections are present on the queue, accept() fails with 
			the error EAGAIN or EWOULDBLOCK.
			
		*/

		if (data_socket == 0) {
			perror("Accept failed");
			exit(EXIT_FAILURE);
		}
		
		// Intialising sum=0
		server_result = 0;
	
		// Reading client data unless client hits 0 (zero)
		while(1) {
			// Initialising buffer with 0
			memset(buffer, 0 , BUFFER_SIZE);

			printf("Waiting for data from the client...\n");
			
			// Read client data in the buffer
			ret = read(data_socket, buffer, BUFFER_SIZE);

			if (ret == -1) {
				perror("Read failed");
				exit(EXIT_FAILURE);
			}

			// Copying each int value sent by client from buffer to data variable
			memcpy(&client_data, buffer, sizeof(int));
			
			printf("Read %d from client...\n", client_data);
			
			if(client_data == 0) break;
			
			server_result += client_data;
		}

		// Sending back the result to client
		
		// Reinitialising buffer
		memset(buffer, 0, BUFFER_SIZE);
		// Adding contents that are to be sent to client in buffer
		sprintf(buffer, "Result = %d", server_result);
		
		printf("Sending final result back to client...\n");
		ret = write(data_socket, buffer, BUFFER_SIZE);

		if(ret == -1) {
			perror("Write failed");
			exit(EXIT_FAILURE);
		}
		
		// Closing the data_socket
		printf("Closing client connection...\n");
		close(data_socket);

	}

	close(connection_socket);
	printf("Server Connection closed...\n");

	unlink(SOCKET_NAME);
	exit(EXIT_SUCCESS);
}
