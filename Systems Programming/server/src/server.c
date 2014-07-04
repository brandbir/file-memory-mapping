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
#include <signal.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <linux/mman.h>



FILE * file_x = NULL;
char * SHMNAME = "shm";

int client_conn;
void sig_handler(int sig)
{
	printf("Writing to the socket pid: %d, signal: %d\n", getpid(), sig);
	char * code = "$$!*!$$";
	write(client_conn, code, strlen(code));
}

typedef struct client
{
	int pid; //pid
	char *pathname;
	struct client *nextclient;
} client;

//prototype
client* DeleteClient(client* head, int id);
void *mremap(void *old_address, size_t old_size, size_t new_size, int flags);


int main(int argc, char *argv[])
{
	pid_t pid;
	int buff_size = 1024;
	char buff[buff_size];
	int listen_fd;
	struct sockaddr_in serv_addr;
	int server_port = 5001;
	char remote_file[255];

	int bytes_read, fd, table_count = 1;
	struct flock lock;


	//**********************************************
	static client* rootclient;
	client* traversor;

	shm_unlink("shm");
	int shmfd = shm_open(SHMNAME,O_RDWR,0755);
	if (shmfd < 0)
	{
		if (errno == 2)
		{
			shmfd = shm_open(SHMNAME, O_CREAT | O_RDWR, 0755);
			if (shmfd < 0)
			{
				perror("Failure in shm_open()");
				fprintf(stderr, "Error: %d\n", errno);
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			perror("Failure in shm_open()");
			fprintf(stderr, "Error: %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}
	if(shmfd == -1)
	{
		perror("Failed to map shared memory");
		return 0;
	}

	ftruncate(shmfd, sizeof(client));
	rootclient = mmap(NULL, sizeof(client), PROT_READ | PROT_WRITE, MAP_SHARED ,shmfd , 0);

	if(rootclient == MAP_FAILED)
	{
		printf("MMap failed xbiiijn\n");
	}
	client *traversor2; //for table traversing

	rootclient = (client*) malloc(sizeof(client));
	rootclient->pid = 0;
	rootclient->pathname = "";
	rootclient->nextclient = 0;
	//*****************************************

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

	int client = 0;
	while(1)
	{
		//Accepting client connection
		client_conn = accept(listen_fd, (struct sockaddr *) NULL, NULL);

		printf("\nPrinting the list Parent!!! where count is : %d\n", table_count);
		traversor2 = rootclient;
		while (traversor2 != NULL)
		{
			printf("\nTable : %d   %s\n", traversor2->pid, traversor2->pathname);
			traversor2 = traversor2->nextclient;
		}
		client++;
		printf("connected clients: %d and connected socket descriptor: %d\n", client, client_conn);
		if (client_conn < 0)
		{
			perror("Client was not accepted...");
			exit(1);
		}

		if((pid = fork()) == 0)
		{
			signal(SIGUSR1, sig_handler);
			int update_cycles = 0;
			int first_pass = 1;
			//Child process closes listening socket
			close(listen_fd);
			bzero(buff, buff_size);


			while((bytes_read = read(client_conn, buff, buff_size)) > 0)
			{
				//printf("LOOP: Buffer received: %s\n\n\n", buff);
				if (bytes_read < 0)
				{
					perror("No data was read from the client");
					exit(1);
				}
				
				//************************************************************************

				if (buff[0] == 'R')
				{
					//deleting client's table entry
					rootclient = DeleteClient(rootclient, client_conn);

					//printing again
					printf("\nPrinting List again\n");
					traversor2 = rootclient;
					while (traversor2 != NULL)
					{
						printf("\nTable: %d   %s\n", traversor2->pid, traversor2->pathname);
						traversor2 = traversor2->nextclient;
					}
				}
				//************************************************************************

				//Holding a copy of the actual buffer
				char *buff_cpy = (char *)malloc(strlen(buff) + 1);
				strcpy(buff_cpy, buff);

				//Updating file for a write request specified by a client
				if(buff[0] == 'U' || update_cycles != 0)
				{
					int cycles, token_size;
					char *token;

					//get the number of cycles we want to perform in order to
					//write all the contents received by the client
					if(buff[0] == 'U')
					{
						token = strtok(buff_cpy, "$");
						token = strtok(NULL, "$");
						token_size = strlen(token);
						cycles = atoi(token);
						update_cycles = cycles;
						free(buff_cpy);
					}

					printf("Updating %s from socket %d\n", remote_file, client_conn);

					//Updating file contents on server
					if(first_pass)
						file_x = fopen(remote_file, "w+");

					else
						file_x = fopen(remote_file, "a+");

					if(file_x == NULL)
					{
						printf("File cannot be opened");
					}

					else
					{
						//Getting file descriptor
						fd = fileno(file_x);
						//Initialisation of flock structure
						memset(&lock, 0, sizeof(lock));
						lock.l_type = F_WRLCK;
						//Placing the write lock on the specified file
						fcntl(fd,F_SETLKW, &lock);

						//removing flags for first pass
						if(first_pass)
						{
							fwrite(buff + (3 + token_size) , sizeof(char), strlen(buff) - (3 + token_size), file_x);
						}
						else
							fwrite(buff, sizeof(char), strlen(buff), file_x);

						lock.l_type = F_UNLCK;
						fcntl(fd, F_SETLKW, &lock);
						fclose(file_x);
					}
				}
				else
				{
					//printf("Received buffer : %s", buff);
					//Obtaining a shared memory key and sending it to the client
					strcpy(remote_file, buff);

					//Appending to the table
					traversor = rootclient;
					printf("\nThis is the client socket descriptor %d", client_conn);

					if (traversor != 0) //moving the traversor to the end of list
					{
						while (traversor->nextclient != 0)
							traversor = traversor->nextclient;
					}

					rootclient->pid = 300;
					//creates a node at the end of list
					table_count++;
					printf("table count : %d\n", table_count);
					/*rootclient = mremap(rootclient, ((table_count - 1) * sizeof(client)), (table_count * sizeof(client)), MREMAP_MAYMOVE);
					if(rootclient == MAP_FAILED)
					{
						printf("errno: %d\n", errno);
						printf("KEEP CALM\n");
					}*/
					traversor->nextclient = malloc(sizeof(client));
					traversor = traversor->nextclient;

					if (traversor == 0)
					{
						printf("Out of memory");
						return 0;
					}

					//Initialising new memory

					printf("\nPrinting the list 1\n");
					traversor2 = rootclient;
					while (traversor2 != NULL)
					{
						printf("\nTable : %d   %s\n", traversor2->pid, traversor2->pathname);
						traversor2 = traversor2->nextclient;
					}
					traversor->pid = getpid();
					traversor->pathname = remote_file;
					traversor->nextclient = 0;

					//printing the list
					printf("\nPrinting the list 2\n");
					traversor2 = rootclient;
					while (traversor2 != NULL)
					{
						printf("\nTable : %d   %s\n", traversor2->pid, traversor2->pathname);
						traversor2 = traversor2->nextclient;
					}

					//**************************************************

					int sh_mem_key = ftok(remote_file, 1);
					char buffer[buff_size];
					bzero(buffer, buff_size);
					sprintf(buffer, "%d", sh_mem_key);
					strcat(buffer, "$");

					//sending key to the client in order to get a shared memory specified by this key with an '*' as a delimiter
					write(client_conn, buffer, strlen(buffer));

					//Opening the file specified by the client
					printf("\nOpening %s...\n\n", remote_file);
					FILE *file = fopen(remote_file, "rb");

					if(file == NULL)
					{
						printf("File %s cannot be opened...\n", remote_file);
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
							printf("Sending %d bytes through socket %d...\n", bytes_read, client_conn);
							write(client_conn, buffer, bytes_read);
						}

						if(bytes_read < buff_size)
						{
							if(ferror(file))
								printf("Read Error\n");

							break;
						}
					}

					fclose(file);
				}
				bzero(buff, buff_size);

				if(update_cycles == 1)
				{
					//updating clients
					//********************************************************************************
					//loop through the linked list to find the clients with same file
					//char update[] = "U$";
					traversor2 = rootclient;

					while (traversor2 != NULL)
					{
						if (strcmp(traversor2->pathname, remote_file) == 0)
						{
							printf("\nClients to update : %d   %s\n", traversor2->pid, traversor2->pathname);
							//sending signal to update the corresponding client
							kill(traversor2->pid, SIGUSR1);
						}

						traversor2 = traversor2->nextclient;
					}

					update_cycles--;
					//write(client_conn, update, strlen(update));
					//**************************************************************************************
				}

				else if(update_cycles != 0)
				{
					update_cycles--;
					first_pass = 0;
				}
			}

			//Terminating child process and closing socket
			close(client_conn);
			exit(0);

			bzero(buff, buff_size);
			update_cycles++;
		}

		//parent process closing socket connection
		close(client_conn);
	}

	return 0;
}

client* DeleteClient(client* head, int socket_desc)
{
	//checking if we are at the end of list
	if (head == NULL)
		return NULL;

	//checking to see if the current node is one to be deleted
	if (head->pid == socket_desc)
	{
		client* tempNextP = malloc(sizeof(client));

		tempNextP = head->nextclient; //save the next ptr in the node
		free(head); //deallocating the node

		//return the new pointer to where we called from. i.e
		// * the pointer call will use to "skip over" the removed node.
		return tempNextP;
	}

	//check the rest of the list, fixing the next pointer in tcase the next nose
	//is the the one removed.
	head->nextclient = DeleteClient(head->nextclient, socket_desc);

	//return the ptr to where we were called from. since we did not remove this node it will be the same
	return head;
}
