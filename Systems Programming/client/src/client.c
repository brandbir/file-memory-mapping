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
    char *buff = NULL;
	struct in_addr server_addr;
	inet_aton(IP, &server_addr);
	int server_port = htons(PORT);

	fileloc_t location;
	location.ipaddress = server_addr;
	location.port = server_port;
	location.pathname = FILENAME;

	/*
	 * Returning a pointer to the memory allocated for the file
	 * This points to an array, in which it contains a sequential list
	 * of pointers to file chucks that are currently grouped in [256 bytes]
	 */
	void *address = rmmap(location, 500);

	if (address == (void*) -1)
	{
		printf("File was not mapped correctly...\n");
		exit(-1);
	}
	else
	{
		printf("\nFile contents: \n");
		printf("%s\n\n", (char *)address);
	}

	//Deallocation of mapped memory
	rmunmap(address);

    /*ssize_t bytesRead = mread(address, 5, buff, 10);
    printf("\n These are the number of bytes read : %ji\n", bytesRead);

    ssize_t bytesRead2 = mread(address, 10, buff, 13);
    printf("\n These are the number of bytes read : %ji\n", bytesRead2);*/

    buff = "Replacing text in memory";
    ssize_t bytesWritten = mwrite(address, 110, buff, 11);
    printf("\n These are the number of bytes read : %ji\n", bytesWritten);

	return 0;
}//
