#include "my_header.h"

/*
    Echo server program. Specs: STREAM_SOCKET, Internet Domain, TCP Connection.
    
    Algorithm: 
    1. Create a SOCK_STREAM socket on Internet Domain
    2. Bind that socket on the local host machine's MY_PORT
    3. Start Listening for connections
    4. Receive Connection in a Loop
        Handle individual connection
        close the connection's socket descriptor
*/

void handle_connection(int);

int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addrlen = sizeof(struct sockaddr_storage);
    int sockfd, new_sockfd;
    char ip[INET6_ADDRSTRLEN];

    sockfd = bind_to_port(AF_UNSPEC, SOCK_STREAM, MY_PORT); // socket & bind calls in this function

    if (listen(sockfd, BACKLOG) == -1) 
    { // listen call
        perror("server listen");
        return 3;
    }
    printf("Waiting for connections on port %s...\n\n", MY_PORT);

    while (1) 
    { // Accept Loop
        addrlen = sizeof(struct sockaddr_storage);
        if ((new_sockfd = accept(sockfd, (struct sockaddr *) &their_addr, &addrlen)) == -1) 
        {
            perror("server accept: ");
            continue;
        }

        // Print peer IP from their_addr
        inet_ntop(their_addr.ss_family, get_sin_addr((struct sockaddr *) &their_addr), ip,
                  INET6_ADDRSTRLEN);
        printf("server: Established connection with %s\n", ip);

        handle_connection(new_sockfd);
        close(new_sockfd);
    }
}

void handle_connection(int sockfd)
{
    /*
        1. Read data from the sockfd
        2. Send the same data back to the socket
    */
    int bytes_received;
    char buf[BUFF_SIZE];
    while ((bytes_received = recv(sockfd, buf, BUFF_SIZE, 0)) > 0)
    {
        int len = bytes_received;
        if (send_all(sockfd, buf, &len) == -1)
        {
            perror("server send");
            printf("Managed to send %i out of %i data\n", len, bytes_received);
        }
        printf("Successfully sent %i bytes\n", len);
    }
    if (bytes_received == -1)
        perror("server: recv");
    else // recv returned 0
        printf("Remote side has closed the connection\n");
    return;
}