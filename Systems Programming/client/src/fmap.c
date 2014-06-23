#include "fmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include<fcntl.h>
#include <errno.h>
#include <sys/mman.h>


void *rmmap(fileloc_t location, off_t offset)
{
	fileloc_t *memory_mapped = (fileloc_t *)malloc(sizeof(fileloc_t));

	//Memory allocated for file read from server
	char *mapped_file = NULL;
	int map_size = 0;

	//Buffer for contents read from server
	int buff_size = 256;
	char buffer[buff_size];
	struct hostent *server;
	int bytes_rec;
	struct sockaddr_in serv_addr;

	//Opening a socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

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

	// Connect to the server */
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

	//Creating a file to which data will be stored for testing purposes
	FILE *file = fopen("/Users/andrewcarlsammut/Dropbox/AssignmentSystemsProgramming/client/client/read.txt", "w+");

	if(file == NULL)
		printf("File cannot be opened");

	bzero(buffer, buff_size + 1);
	printf("Mapping file to memory...\n");

	//Receiving file contents from server
	while((bytes_rec = read(sockfd, buffer, buff_size)) > 0)
	{
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

			//Writing contents to file
			fwrite(buffer, 1, bytes_rec, file);
		}
		else
		{
			offset -= bytes_rec;
		}

		bzero(buffer, buff_size + 1);
	}

	if(mapped_file == NULL)
	{
		printf("Nothing was read from the remote file...\n");
		mapped_file = (void *)-1;
	}

	close(sockfd);
	memory_mapped->ipaddress = location.ipaddress;
	memory_mapped->port = location.port;
	memory_mapped->pathname = mapped_file;
	return memory_mapped;
}

int rmunmap(void *addr, size_t length)
{
	fileloc_t *m = (fileloc_t *)addr;
	addr = m->pathname;
	if(addr == (void*)-1)
	{
		return -1;
	}
	else
	{
		free(addr);

		return 0;
	}
}

// Attempt to read up to count bytes from memory mapped area pointed
// to by addr + offset into buff
//
// returns: Number of bytes read from memory mapped area
ssize_t mread(void *addr, off_t offset, void *buff, size_t count)
{

  
    buff = (char*)malloc(count * sizeof(char *));
    //use memcpy to copy contents from address in buf
    if(memmove(buff,addr+offset,count) < 0) // used memove to handle cases when dst and src may overlap
    {
        perror("Cannot Read!");
    }

    printf("\nThese are the contents in buffer : %s\n", buff);
    //printf("\n these are the contents in address %s\n", addr);

    bzero(buff, count * sizeof(char));
    printf("\nSize of Buffer %lu\n ", sizeof(*buff) * count);

	return count;
}


// Attempt to write up to count bytes to memory mapped area pointed
// to by addr + offset from buff
//
// returns: Number of bytes written to memory mapped area
ssize_t mwrite(void *addr, off_t offset, void *buff, size_t count)
{
	//Retrieving the actual memory address of the mapped file
	fileloc_t *file_mapped = (fileloc_t *)addr;
	addr = file_mapped->pathname;
	struct sockaddr_in serv_addr;


    printf("\nThese are the contents in address :\n%s\n", addr);
    printf("\nThis the size of the address :\n%lu\n", strlen((char*)addr));
    int memorySize = (int)strlen((char*)addr);

    if(offset > memorySize)
    {
        perror("Offset out of range!");
    }

    if((offset+count) <= memorySize)  //overwriting
    {
        //overwriting the file
        if(memmove(addr+offset, buff, count) ==NULL) //returns ptr to dest
        {
            perror("Cannot copy the buffer");
        }
    }
    else if((offset + count) >memorySize)
    {
        int difference = (offset + count) - memorySize;
        addr = (char *)realloc(addr, difference + memorySize);
        if(offset == memorySize) //appending to the end of the file
        {
            char* substr = (char*)malloc(count);
            strncpy(substr, buff, count);
            strcat(addr, substr);
            printf("\nThese are the contents in address AFTER WRITE :\n%s\n", addr);
            return count;
        }
        else //overwriting + appending!
        {
            int remainingSpace = memorySize - offset;
            printf("\nThese are the contents in buffer :%s\n", buff);
            if(memcpy(addr + offset, buff, remainingSpace) == NULL) //return ptr to dest
            {
                perror("Cannot copy the buffer");
            }
            char* substr = (char*)malloc(count- remainingSpace); // strlen((char*)addr) - remainingSpace
            strncpy(substr, buff + remainingSpace, count - remainingSpace);  //adding last part of string
            strcat(addr, substr);

           // printf("\nThese are the contents in address AFTER WRITE :\n%s\n", addr);
           
        }

    }

    printf("\nThese are the contents in address AFTER WRITE :\n%s\n", addr);

//After writing to memory we need to synchronise with the server
	//Buffer for contents read from server
	struct hostent *server;

	//Opening a socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	//Check for Socket Error
	if (sockfd < 0)
	{
		perror("Socket cannot be opened");
		exit(1);
	}

	//Getting server details
	server = gethostbyaddr(&file_mapped->ipaddress, sizeof(file_mapped->ipaddress), AF_INET);
	if (server == NULL )
	{
		fprintf(stderr, "Server Authentication failed\n");
		exit(0);
	}

	// Populate serv_addr structure
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server-> h_addr, (char *) &serv_addr.sin_addr.s_addr, server -> h_length);
	serv_addr.sin_port = file_mapped->port;

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Connection Refused");
		exit(1);
	}

	char * updated_memory = (char *)addr;
	char * prefix = "U"; // Informing the server that this is an Update
	char * message = (char *)malloc(strlen(updated_memory + 2));
	strcpy(message, prefix);
	strcat(message, updated_memory);
	//sending the updated version to the server
	int bytes_written = write(sockfd, message, strlen(message));
	if (bytes_written < 0)
	{
		perror("File was not updated to the server");
		exit(1);
	}
    
	return count; 
}
