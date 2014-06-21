#include "fmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

void *rmmap(fileloc_t location, off_t offset)
{
	char *file_part[200];
	char buffer[256];
	struct hostent *server;
	int bytes_rec;
	struct sockaddr_in serv_addr;

	// Create a socket point
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// Check for errors
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}

	// Get server name
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

	//Creating a file to which data will be stored
	FILE *file = fopen("read.txt", "w+");

	if(file == NULL)
		printf("File cannot be opened");

	//Receiving file contents from server
	bzero(buffer, 257);
	int mem_count = 0;
	//set_memory(&memory_map, 5);

	printf("Mapping file to memory...\n");
	while((bytes_rec = read(sockfd, buffer, 256)) > 0)
	{
		file_part[mem_count] = (char *)malloc((bytes_rec) * sizeof(char));
		strcpy(file_part[mem_count++], buffer);
		fwrite(buffer, 1, bytes_rec, file);
		bzero(buffer, 257);
	}

	if(bytes_rec < 0)
	{
		printf("Read error");
	}
	else
	{
		int i;
		for(i = 0; i < mem_count; i++)
		{
			printf("%s", file_part[i]);
		}

		printf("\n_____________________________________________________________________________________________________\n");
		printf("File Mapping: \n\n");
		for(i = 0; i < mem_count; i++)
		{
			printf("%d) %p\n", i, file_part[i]);
		}

		printf("_____________________________________________________________________________________________________\n");
	}

	close(sockfd);

	return file_part;
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
