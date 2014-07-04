#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "fmap.h"

#define IP "127.0.0.1"

#define PORT 5001
#define FILENAME "file.txt"

int main(int argc, char *argv[])
{
	printf("Client %d Running\n", getpid());
	char *buff = NULL;
	struct in_addr server_addr;
	inet_aton(IP, &server_addr);
	int server_port = htons(PORT);

	fileloc_t location;
	fileloc_t *mapped_location;

	location.ipaddress = server_addr;
	location.port = server_port;
	location.pathname = FILENAME;

	void *address = rmmap(location, 0);

	if (address == (void*) -1)
	{
		printf("Nothing was mapped from remote file\n");
		exit(-1);
	}
	else
	{
		printf("\nMemory Mapped Contents:");
		mapped_location = (fileloc_t *)address;
		printf("%s\n\n", mapped_location->pathname);
	}

	buff = "b";
	mwrite(address, 0, buff, strlen(buff));

	mapped_location = (fileloc_t *)address;
	printf("Updated Memory Content:%s\n\n", mapped_location->pathname);

	//Deallocation of mapped memory
	rmunmap(address);

	return 0;
}
