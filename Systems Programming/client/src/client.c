#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    // Declare variables
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    // Make sure that server name and port are available in command line arguments
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }

    // Get port number
    portno = atoi(argv[2]);

    // Create a socket point
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Check for errors
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // Get server name
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    // Populate serv_addr structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  // Set to AF_INET
    bcopy((char *) server -> h_addr, // Set server address
          (char *) &serv_addr.sin_addr.s_addr,
                   server -> h_length);
    serv_addr.sin_port = htons(portno); // Set port (convert to network byte ordering)

    // Connect to the server */
    if (connect(sockfd,&serv_addr,sizeof(serv_addr)) < 0)
    {
         perror("ERROR connecting");
         exit(1);
    }

    // Ask for a message from the user
    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    // Send message to the server
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
    {
         perror("ERROR writing to socket");
         exit(1);
    }

    // Read response from server response
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
    {
         perror("ERROR reading from socket");
         exit(1);
    }
    printf("%s\n",buffer);

    // All done, close socket
    close(sockfd);
    return 0;
}
