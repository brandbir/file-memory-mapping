#include "fmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <linux/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>

struct address
{
	struct in_addr ipaddress;
	int port;
	char *memory_address;
	int shm_id;
};

//prototypes
void *mremap(void *old_address, size_t old_size, size_t new_size, int flags);
typedef struct address address_t;

//mutex variables for handling synchronisation
sem_t *mutex;
char * SEM_NAME = "mtx";
int sockfd;
int buff_size = 1024;

void *rmmap(fileloc_t location, off_t offset)
{
	address_t *memory_mapped = (address_t *)malloc(sizeof(address_t));
	char *mapped_file = NULL;
	int map_size = 0;
	int sh_mem_id;
	char *shm;
	int sub_buff_len;

	//Buffer for contents read from server
	char buffer[buff_size];
	struct hostent *server;
	int bytes_rec;
	struct sockaddr_in serv_addr;

	//Opening a socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//Check for Socket Error
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}

	//Getting server details
	server = gethostbyaddr(&location.ipaddress, sizeof(location.ipaddress), AF_INET);
	if (server == NULL )
	{
		fprintf(stderr, "Authentication failed...\n");
		exit(0);
	}

	// Populate serv_addr structure
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server-> h_addr, (char *) &serv_addr.sin_addr.s_addr, server -> h_length);
	serv_addr.sin_port = location.port;

	// Connect to the server
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Connection Refused");
		exit(1);
	}

	//Sending the file_path to be mapped
	int bytes_written = write(sockfd, location.pathname, strlen(location.pathname));
	if (bytes_written < 0)
	{
		perror("File path was not successfully sent to the server...");
		exit(1);
	}

	bzero(buffer, buff_size + 1);
	int sh_mem_key_size;

	if((sh_mem_key_size = read(sockfd, buffer, buff_size)) < 0)
		perror("shared memory key was not obtained from the server");

	//Finding delimiter after received key number from the server
	char * sub_buff = strchr(buffer, '$');

	if (sub_buff != NULL)
	{
		//found the delimiter between key and file contents
		//This means that we need to separate the buffer contents
		sub_buff = sub_buff + 1; //eliminating the delimiter
		sub_buff_len = strlen(sub_buff);
	}

	//converting received key to an integer
	int key = atoi(buffer);
	int count = 0;

	//Getting number of digits contained in key
	while(key != 0)
	{
		key = key / 10;
		++count;
	}
	if(sub_buff_len == 0)
		bzero(buffer, buff_size + 1);

	printf("Mapping file to memory...\n");

	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	sleep(1);

	//Receiving file contents from server
	while(sub_buff_len > 1 || ((bytes_rec = read(sockfd, buffer, buff_size)) > 0))
	{
		if(sub_buff_len > 1)
		{
			//update buffer in order to handle already received file contents
			memmove (buffer, buffer + (count + 1), strlen(buffer) + (count + 1));
			bytes_rec = sub_buff_len;
			sub_buff_len = 0;
		}

		if(offset <= bytes_rec)
		{
			if(mapped_file == NULL)
			{
				//Initialising memory for file mapping
				mapped_file = (char *)malloc((bytes_rec) * sizeof(char));
				memmove (buffer, buffer + offset, strlen(buffer + 1));
				strcpy(mapped_file, buffer);
				map_size += bytes_rec;
			}

			else
			{
				//Reallocating memory to fit the contents of the file
				mapped_file = (char *)realloc(mapped_file, map_size + bytes_rec);
				map_size += bytes_rec;
				strcat(mapped_file, buffer);
			}
		}
		else
		{
			offset -= bytes_rec;
		}

		bzero(buffer, buff_size + 1);
	}

	/*Trying to attach with an already obtained shared memory on the client side.
	This means that if an other client has already gained access to the shared memory
	specified by the received key, the current process must attach to this shared memory,
	otherwise this process needs to get the shared memory segment id and thereafter
	attach the memory segment*/

	//No memory was allocated for contents that reside in the remote file.
	//This implies that the remote file is empty
	if(mapped_file != NULL)
	{
		//Creating/Obtaining the segment and set read/write permissions
		if((sh_mem_id = shmget(key, strlen(mapped_file), IPC_CREAT | 0666)) < 0)
			perror("shared memory segment was not allocated");

		printf("Shared Memory id: %d", sh_mem_id);
		//Attachment of segment with the malloc'd data space
		if((shm = shmat(sh_mem_id, NULL, 0)) != (void *)-1)
		{
			strcpy(shm, mapped_file);
		}

		else
			perror("Shared memory was not attached\n");

		//Deallocation of mapped file after allocating
		//the mapped file into shared memory segment
		free(mapped_file);
		memory_mapped->ipaddress = location.ipaddress;
		memory_mapped->port = location.port;
		memory_mapped->memory_address = shm;
		memory_mapped->shm_id = sh_mem_id;
	}
	else
	{
		memory_mapped = (void *)-1;
	}

	return memory_mapped;
}

int rmunmap(void *addr)
{
	struct shmid_ds shm_desc;
	address_t *address = (address_t *)addr;
	addr = address->memory_address;
	if(addr == (void*)-1)
	{
		return -1;
	}
	else
	{
		//removing the shared memory segment until no process is using the shared memory
		if(shmctl(address->shm_id, IPC_RMID, &shm_desc) == -1)
			perror("shared memory not destroyed");

		//closing mutex
		sem_close(mutex);
		sem_unlink(SEM_NAME);
		close(sockfd);
		return shmdt(addr);
	}
}

// Attempt to read up to count bytes from memory mapped area pointed
// to by addr + offset into buff
//
// returns: Number of bytes read from memory mapped area
ssize_t mread(void *addr, off_t offset, void *buff, size_t count)
{

	buff = (char*) malloc(count * sizeof(char *));
	//use memcpy to copy contents from address in buf
	if (memmove(buff, addr + offset, count) < 0) // used memove to handle cases when dst and src may overlap
	{
		perror("Cannot Read!");
	}

	bzero(buff, count * sizeof(char));
	printf("\nSize of Buffer %lu\n ", sizeof(*buff) * count);

	return count;
}

/*
 * 	Attempt to write up to count bytes to memory mapped area pointed to by addr + offset from buff
 *	returns: Number of bytes written to memory mapped area
*/
ssize_t mwrite(void *addr, off_t offset, void *buff, size_t count)
{
	//Retrieving the actual memory address of the mapped file
	address_t *file_mapped = (address_t *) addr;
	addr = file_mapped->memory_address;
	//struct sockaddr_in serv_addr;
	int written_mem = count;
	int mem_size = (int) strlen((char*) addr);

	if (offset > mem_size)
	{
		perror("Offset is out of range");
		written_mem = -1;
	}

	//Overwriting
	int mem_range;
	if ((mem_range = offset + count) <= mem_size)
	{
		//Overwriting the mapped memory
		if (memmove(addr + offset, buff, count) == NULL)
		{
			perror("Cannot copy the buffer");
			written_mem = -1;
		}
	}
	else if (mem_range > mem_size)
	{
		//Exceeding the memory allocated, therefore we need
		//to reallocate extra space in order to write new
		//data in the specified region, including the null terminator

		//Handling synchronisation for multiple writes by using mutex
		//in relation with shared memory

		//creating mutex that handles synchronisation between processes
		//using the same shared memory

		mutex = sem_open(SEM_NAME, O_CREAT, 0644, 1);
		if(mutex == SEM_FAILED)
		{
			perror("Semaphore was not created");
			sem_unlink(SEM_NAME); //unlink semaphore
			exit(-1);
		}
		sem_wait(mutex);
		addr = mremap(addr, mem_size, (mem_range + 1), MREMAP_MAYMOVE);
		strcpy(addr + offset, buff);
		sem_post(mutex);

	}

	//After writing to memory we need to synchronise with the server
	//Buffer for contents read from server

	//Updated Memory + U (for Update) and a digit to
	//show how many times the server needs to traverse
	//the buffer, in the case when it exceeds the buffer size

	int message_size = strlen(addr) + 2;
	char cycles[10];
	int num_of_cycles = (message_size + buff_size - 1) / buff_size;
	sprintf(cycles, "%d", num_of_cycles);
	char * updated_memory = (char *)addr;
	//Informing the server that this is an Update

	//Allocating memory for the prefix (U + 2 $ symbols and number of cycles)
	char *prefix = (char *)malloc(sizeof(char) * (3 + strlen(cycles)));
	strcpy(prefix, "U$");
	strcat(prefix, cycles);
	strcat(prefix, "$");
	char * message = (char *)malloc(strlen(updated_memory + 3));
	strcpy(message, prefix);
	strcat(message, updated_memory);

	//Sending the updated version to the server
	//printf("\nMessage to be sent: %s\n\n\n", message);
	int bytes_written = write(sockfd, message, strlen(message));
	if (bytes_written < 0)
	{
		perror("Mapped file is not going to be updated on the Remote File");
		exit(1);
	}

	char code[7] = "       ";
	sleep(1);
	int bytes_read = read(sockfd, code, strlen(code));
	printf("Buffer read after mwrite : %s\n", code);
	return written_mem;
}
