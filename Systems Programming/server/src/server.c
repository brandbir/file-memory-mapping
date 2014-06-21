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
	pid_t pid;
	struct sockaddr_in serv_addr;
	char file_name[255];

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
	printf("Process %d is waiting for a connection...\n", getpid());

	while(1)
	{
		//Accepting client connection
		client_conn = accept(listen_fd, (struct sockaddr *) NULL, NULL);

		if (client_conn < 0)
		{
			perror("Client was not accepted...");
			exit(1);
		}


		if((pid = fork()) == 0)
		{
			//Child process closes listening socket
			close(listen_fd);
			bzero(file_name, 255);
			int bytes_read = read(client_conn, file_name, 255);

			if (bytes_read < 0)
			{
				perror("No data was read from the client");
				exit(1);
			}
			printf("\nOpening %s...\n", file_name);


			//Opening the file specified by the client
			FILE *file = fopen(file_name, "rb");
			if(file == NULL)
			{
				printf("File %s cannot be opened...\n", file_name);
				return -1;
			}

			printf("Data Transfer\n");
			while(1)
			{
				char buffer[256];
				bzero(buffer, 256);
				int bytes_read = fread(buffer, 1, 256, file);

				if(bytes_read > 0)
				{
					printf("  Sending %d bytes to the client...\n", bytes_read);
					write(client_conn, buffer, bytes_read);
				}

				if(bytes_read < 256)
				{
					if(ferror(file))
						printf("Read Error\n");

					break;
				}
			}

			//Terminating child process and closing socket
			close(client_conn);
			exit(0);
		}
		//parent process closing socket connection
		close(client_conn);
	}

	return 0;
}
