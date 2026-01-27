#include "my_header.h"

// Creates a socket of family and socktype and binds it at a local port. Returns the socket 
// descriptor. On fatal error, the calling program program exits.
int bind_to_port(int family, int socktype, const char *port)
{
    int status, sockfd, yes;
    struct addrinfo hints, *p, *res;
    yes = 1;

    // Prepare the getaddrinfo call
    memset(&hints, '\0', sizeof hints);
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;

    // Make getaddrinfo call
    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "%s\n", gai_strerror(status));
        exit(1);
    }

    // Search linked list and bind to the first socket address you can.
    for (p = res; p != NULL; p = p->ai_next)
    {
        // try to get socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype, 0)) == -1)
        {
            perror("server socket: ");
            continue;
        }
        // Immediate reuse of addresses in bind (useful on quick program restarts)
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("server setsockopt");
            close(sockfd);
            continue;
        }
        // try to bind socket to a local port
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("server bind: ");
            close(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(res); 

    if (p == NULL)
    {
        fprintf(stderr, "Failed to bind\n");
        exit(2);
    }
    return sockfd;   
}

// Returns a pointer to the IP address structure pointed to by the socket address *p
void *get_sin_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
        return &((struct sockaddr_in *) sa)->sin_addr;
    else 
        return &((struct sockaddr_in6 *) sa)->sin6_addr;
}

int send_all(int sockfd, char *buf, int *len)
{
    int total_sent = 0;
    int bytes_sent;
    while (total_sent < *len)
    {
        // try to send data
        bytes_sent = send(sockfd, buf + total_sent, *len - total_sent, 0);
        if (bytes_sent == -1)
        {
            *len = total_sent;
            return -1;
        }
        // update total_sent
        total_sent += bytes_sent;
    }
    *len = total_sent;
    return 0;
}