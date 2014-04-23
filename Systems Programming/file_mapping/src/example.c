#include "fmmap.h"
#include <stdio.h>
#include <stdlib.h>

#define IP "193.188.47.67"
#define PORT 6000
#define FILENAME "/home/mary/exmaple.txt"

int main()
{
    // Remote location
    struct in_addr server_addr;
    inet_aton(IP, &server_addr);
    int server_port = htons(PORT);

    fileloc_t location;
    location.ipaddress = server_addr;
    location.port = server_port;
    location.pathname = FILENAME;

    // Memory map remote file at offset 0
    void *address = rmmap(location, 0);
    if (address == (void*) -1)
    {
        printf("Something didn't go very well\n");
        exit(-1);
    }

    // Write 1024 bytes to memory mapped region
    ssize_t length = 1024;
    char buff[length];

    if (mwrite(address, 0, buff, length) != length)
    {
        printf("Something doesn't seem right\n");
        rmunmap(address);
        exit(-1);
    }

    // All done, clear memory mapping and exit
    if (!rmunmap(address))
    {
        printf("Couldn't clear memory mapped region\n");
        exit(-1);
    }   

    return 1;
}
