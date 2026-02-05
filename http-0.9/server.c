#include "my_header.h"


int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addrlen = sizeof(struct sockaddr_storage);
    int sockfd, new_sockfd;
    char ip[INET6_ADDRSTRLEN];

    sockfd = bind_to_port(AF_UNSPEC, SOCK_STREAM, MY_PORT); // socket & bind call

    if (listen(sockfd, BACKLOG) == -1) // listen call
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
    
}

