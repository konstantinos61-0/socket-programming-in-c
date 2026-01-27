#include "my_header.h"

int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addrlen = sizeof(struct sockaddr_storage);
    int sockfd, new_sockfd;
    char ip[INET6_ADDRSTRLEN];
    // Create a sockfd according to the first 2 arguements
    // Bind the sockfd to the local MY_PORT and return the sockfd 
    sockfd = bind_to_port(AF_UNSPEC, SOCK_STREAM, MY_PORT);
    // listen call
    if (listen(sockfd, BACKLOG) == -1) 
    {
        perror("server error(listen): ");
        return 3;
    }
    printf("Listening for connections on port %s...\n\n", MY_PORT);

    while (1) 
    { // Accept Loop
        addrlen = sizeof(struct sockaddr_storage);
        if ((new_sockfd = accept(sockfd, (struct sockaddr *) &their_addr, &addrlen)) == -1) 
        {
            perror("server error(accept): ");
            continue;
        }

        // Handle connection
        handle_connection(new_sockfd);
        close(new_sockfd);
    }
}

void handle_connection(new_sockfd) 
{
    char request_buf[BUFFER_SIZE];
    // Recv all of the request into the buffer
    // if it doesnt fit: return error.
    
    // Parse request
    // if there is an error, send error back
    // to client
    // otherwise serve the document
}

