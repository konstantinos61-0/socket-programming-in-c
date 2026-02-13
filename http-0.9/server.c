#include "my_header.h"
/*
    Usage: server root_dir [port]
*/

int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addrlen;
    int server_sockfd, client_sockfd, root_dir;
    char *port;
    char ip[INET6_ADDRSTRLEN];

    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "Usage: %s root_dir [port]\n", argv[0]);
        return 1;
    }

    // Port configuration
    if (argc == 2)
        port = MY_PORT;
    else
    {
        port = argv[2];
    }

    // Root directory configuration
    if ((root_dir = open(argv[1], O_RDONLY)) == -1)
    {
        perror("root dir open error");
        return 3;
    }

    if ((server_sockfd = bind_to_port(AF_UNSPEC, SOCK_STREAM, port)) == -1) // socket & bind call
    {
        return 4;
    } 

    if (listen(server_sockfd, BACKLOG) == -1) // listen call
    {
        perror("server error(listen): ");
        return 4;
    }
    printf("Listening for connections on port %s...\n\n", MY_PORT);

    while (1) 
    { // Accept Loop, blocking call socketfd mode
        addrlen = sizeof(struct sockaddr_storage);
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *) &their_addr, &addrlen)) == -1) 
        {
            perror("server error(accept): ");
            continue;
        }
        if (their_addr.ss_family == AF_INET)
        {
            inet_ntop(AF_INET, get_sin_addr((struct sockaddr *)&their_addr), ip, INET6_ADDRSTRLEN);
            printf("Connection Established with client at address: %s\n", ip);
        }
        else
        {
            inet_ntop(AF_INET6, get_sin_addr((struct sockaddr *)&their_addr), ip, INET6_ADDRSTRLEN);
            printf("Connection Established with client at address: %s\n", ip);
        }

        // Handle connection
        handle_connection(client_sockfd, root_dir);
        close(client_sockfd);
        printf("Closed connection with %s\n", ip);
    }

    close(server_sockfd);
    close(root_dir);
    return 0;
}



