#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	// Declare variables
	int listen_fd, client_conn;
	struct sockaddr_in serv_addr;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_fd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}

	// Initialise socket structure
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
	serv_addr.sin_port = htons(5001);

	// Bind the host address
	if (bind(listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}

	// Start listening for the clients, here process will
	// go in sleep mode and will wait for the incoming connection
	listen(listen_fd, 5);

	while(1)
	{
		printf("%s \n", "Waiting for connections...");
		//Accepting client connection
		client_conn = accept(listen_fd, (struct sockaddr *) NULL, NULL);
		if (client_conn < 0)
		{
			perror("ERROR on accept");
			exit(1);
		}

		//Opening the file located on the server
		FILE *file = fopen("file.txt", "rb");
		if(file == NULL)
		{
			printf("File cannot be opened");
			return 1;
		}

		while(1)
		{
			char buffer[256];
			bzero(buffer, 256);
			int bytes_read = fread(buffer, 1, 256, file);
			printf("Number of bytes read %d \n", bytes_read);

			if(bytes_read > 0)
			{
				printf("Sending data to the client\n");
				write(client_conn, buffer, bytes_read);
			}

			if(bytes_read < 256)
			{
				if(feof(file))
					printf("End of file\n");

				if(ferror(file))
					printf("Read Error\n");

				break;
			}
		}

		//Closing client connection
		close(client_conn);
	}

	return 0;
}
