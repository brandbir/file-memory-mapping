#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>

int main(int argc, char *argv[])
{
	pid_t pid;
	int buff_size = 256;
	char buff[buff_size];
	int listen_fd, client_conn;
	struct sockaddr_in serv_addr;
	int server_port = 5001;
	char *remote_file = "file.txt";

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_fd < 0)
	{
		perror("Socket cannot be opened");
		exit(1);
	}

	/*Turning off address checking in order to allow port numbers to be
	reused before the TIME_WAIT. Otherwise it will not be possible to bind
	in a very short time after the server has been shut down*/
	int on = 1;
	int status = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));

	if (status == -1)
	{
		perror("Failed to Reuse Address on Binding");
	}


	// Initialise socket structure
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
	serv_addr.sin_port = htons(server_port);

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
			bzero(buff, buff_size);
			int bytes_read = read(client_conn, buff, buff_size);

			if (bytes_read < 0)
			{
				perror("No data was read from the client");
				exit(1);
			}

			//Updating file for a write request specified by a client
			if(buff[0] == 'U')
			{
				//Updating file contents on server
				FILE *file = fopen(remote_file, "w+");

				if(file == NULL)
					printf("File cannot be opened");

				fwrite(buff + 1, 1, strlen(buff) - 1, file);
			}
			else
			{
				//Obtaining a shared memory key and sending it to the client
				int sh_mem_key = ftok(remote_file, 1);
				char buffer[buff_size];
				bzero(buffer, buff_size);
				sprintf(buffer, "%d", sh_mem_key);
				//sending key to the client in order to get a shared memory specified by this key
				write(client_conn, buffer, strlen(buffer));

				//Opening the file specified by the client
				printf("\nOpening %s...\n", buff);
				FILE *file = fopen(buff, "rb");

				if(file == NULL)
				{
					printf("File %s cannot be opened...\n", buff);
					return -1;
				}

				printf("Data Transfer\n");

				while(1)
				{
					char buffer[buff_size];
					bzero(buffer, buff_size);
					int bytes_read = fread(buffer, 1, buff_size, file);
					if(bytes_read > 0)
					{
						printf("Sending %d bytes to the client...\n", bytes_read);
						write(client_conn, buffer, bytes_read);
					}

					if(bytes_read < buff_size)
					{
						if(ferror(file))
							printf("Read Error\n");

						break;
					}
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
