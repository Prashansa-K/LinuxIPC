#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_NAME "/tmp/UnixDomainSocket"
#define BUFFER_SIZE 128

int main(int argc, char* argv[]) {
	struct sockaddr_un name;
	int data_socket;
	int ret;
	char buffer[BUFFER_SIZE];

	data_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	if (data_socket == -1) {
		perror("Data socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&name, 0, sizeof(struct sockaddr_un));

	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

	// Connection initiation request
	// Not a blocking sys call
	ret = connect(data_socket, (const struct sockaddr*) &name, sizeof(struct sockaddr_un));

	if (ret == -1) {
                perror("Connect failed");
                exit(EXIT_FAILURE);
        }

	int i;
	// If user enters 0, loop breaks
	do {
		printf("Enter number to send to server: \n");
		scanf("%d", &i);

		ret = write(data_socket, &i, sizeof(int));
		
		if (ret == -1) {
                	perror("Write failed");
                	exit(EXIT_FAILURE);
        	}
		
		printf("Sent %d. Number of bytes = %d. \n", i, ret);
	} while(i);

	memset(buffer, 0, BUFFER_SIZE);

	// Read is a blocking sys call
	ret = read(data_socket, buffer, BUFFER_SIZE);
	if (ret == -1) {
        	perror("Read failed");
        	exit(EXIT_FAILURE);
        }

	printf("Result received from server: %s\n", buffer);

	close(data_socket);
	
	exit(EXIT_SUCCESS);

}
