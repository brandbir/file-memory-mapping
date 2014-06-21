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
	server = gethostbyname("127.0.0.1");
	if (server == NULL )
	{
		fprintf(stderr, "ERROR, no such host\n");
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

	//Creating a file to which data will be stored for testing purposes
	FILE *file = fopen("read.txt", "w+");

	if(file == NULL)
		printf("File cannot be opened");

	bzero(buffer, 257);
	printf("Mapping file to memory...\n");

	//Receiving file contents from server
	while((bytes_rec = read(sockfd, buffer, 256)) > 0)
	{
		if(mapped_file == NULL)
		{
			//Initialising memory for file mapping
			mapped_file = (char *)malloc(bytes_rec * sizeof(char));
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
		bzero(buffer, 257);
	}

	if(bytes_rec < 0)
	{
		printf("Read error");
	}
	else
	{
		printf("\nFile Contents: \n");
		printf("%s\n", mapped_file);
		printf("\nMemory Address: %p\n", mapped_file);
	}

	close(sockfd);
	return mapped_file;
}

int rmunmap(void *addr)
{
	return -1;
}

ssize_t mread(void *addr, off_t offset, void *buff, size_t count)
{
	return 0;
}
ssize_t mwrite(void *addr, off_t offset, void *buff, size_t count)
{
	return 0;
}
