#include "fmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

void *rmmap(fileloc_t location, off_t offset)
{
	//Memory allocated for file read from server
	char *mapped_file = NULL;
	int map_size = 0;

	//Buffer for contents read from server
	char buffer[256];
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
	FILE *file = fopen("read.txt", "w+");

	if(file == NULL)
		printf("File cannot be opened");

	bzero(buffer, 257);
	printf("Mapping file to memory...\n");

	//Receiving file contents from server
	while((bytes_rec = read(sockfd, buffer, 256)) > 0)
	{
		if(offset <= bytes_rec)
		{
			if(mapped_file == NULL)
			{
				//Initialising memory for file mapping
				mapped_file = (char *)malloc((bytes_rec) * sizeof(char));
				memmove (buffer, buffer + offset, strlen(buffer+1));
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

		bzero(buffer, 257);
	}

	if(mapped_file == NULL)
	{
		printf("Nothing was read from the remote file...\n");
		mapped_file = (void *)-1;
	}

	close(sockfd);
	return mapped_file;
}

int rmunmap(void *addr)
{
	if(addr == (void *)-1)
		return -1;
	else
		free(addr);

	return 0;
}

// Attempt to read up to count bytes from memory mapped area pointed
// to by addr + offset into buff
//
// returns: Number of bytes read from memory mapped area
ssize_t mread(void *addr, off_t offset, void *buff, size_t count)
{

    //char* memAddr = (char*)addr;

    //printf("\n these are the contents in address %s !!!!", addr);

    //if(buff == NULL)
    //{
        buff = (char*)malloc(count * sizeof(char *));
        //use memcpy to copy contents from address in buf
        //...memcpy(dest,src,strlen(src) +1)
        if(memmove(buff,addr+offset,count) < 0) // used memove to handle cases when dst and src may overlap
        {
            perror("Cannot Read!");
        }
    //}
    //else
    //{
      //  memcpy(buff,addr+offset,count);
    //}

    printf("\n these are the contents in buffer : %s\n", buff);
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
    printf("\nThese are the contents in address :\n%s\n", addr);
    printf("\nThis the size of the address :\n%lu\n", strlen((char*)addr));
    int memorySize = (int)strlen((char*)addr);

    if(offset > memorySize)
    {
        perror("Offset out of range!");
    }

    if((offset+count) < memorySize)  //overwriting
    {
       //overwriting the file
       memmove(addr+offset, buff, count);
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
            memcpy(addr + offset, buff, remainingSpace);

           // char* substr = (char*)malloc(strlen((char*)addr) - remainingSpace);
            //strncpy(substr, buff, strlen((char*)addr) - remainingSpace);  //adding last part of string
            //strcat(addr, substr);

            printf("\nThese are the contents in address AFTER WRITE :\n%s\n", addr);
        }

    }

    printf("\nThese are the contents in address AFTER WRITE :\n%s\n", addr);

	return count; //
}

ssize_t try(void *addr)
{

    return 0;
}
