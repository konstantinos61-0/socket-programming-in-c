#include "my_header.h"
/*
    Creates a socket of family and socktype and binds it at a local port. Returns the socket 
    descriptor. On fatal error, it prints the error message on stderr and returns -1
*/

int bind_to_port(int family, int socktype, const char *port)
{
    int status, server_sockfd, yes;
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
        return -1;
    }

    // Search linked list and bind to the first socket address you can.
    for (p = res; p != NULL; p = p->ai_next)
    {
        // try to get socket
        if ((server_sockfd = socket(p->ai_family, p->ai_socktype, 0)) == -1)
        {
            perror("server socket: ");
            continue;
        }
        // Immediate reuse of addresses in bind (useful on quick program restarts)
        if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("server setsockopt");
            close(server_sockfd);
            continue;
        }
        // try to bind socket to a local port
        if (bind(server_sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("server bind: ");
            close(server_sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(res); 

    if (p == NULL)
    {
        fprintf(stderr, "Failed to bind\n");
        return -1;
    }
    return server_sockfd;   
}

// Returns a pointer to the IP address structure pointed to by the socket address *p
void *get_sin_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
        return &((struct sockaddr_in *) sa)->sin_addr;
    else 
        return &((struct sockaddr_in6 *) sa)->sin6_addr;
}


/*
    Attempt to send len ammount of bytes from buf to sockfd through a the network connection
    At success return 0. At failure return -1. In any case, len is set to the number of bytes actually sent.
*/
int send_all(int client_sockfd, char *buf, int *len)
{
    int bytes_total = 0;
    int bytes_sent;
    while (bytes_total < *len)
    {
        // try to send data
        bytes_sent = send(client_sockfd, buf + bytes_total, *len - bytes_total, 0);
        if (bytes_sent == -1)
        {
            *len = bytes_total;
            return -1;
        }
        // update bytes_total
        bytes_total += bytes_sent;
    }
    *len = bytes_total;
    return 0;
}


/*
    Reads the HTTP/0.9 request into a static sized buffer and sets *len to the actual size of the request.
    On error, prints error message on stderr and returns -1
    An error happens if one of the below happen:
        1. Buffer got full before encoutering the end of a request
        2. Client closed connection before encountering the end of a request
        3. Got error from recv

    1. While (total bytes read < buffer length && there are bytes available in the socket stream)
        2. Read the bytes into the buffer by only loading them after the last read character of last loop's repetition.
        3. If CRLF is found in the buffer
            4. Set *len to the total bytes and return 0;
    5. If buffer is full, print error on stderr and return -1
    6. If recv caused an error, print error on stderr, return -1
    7. Else if client closed connection, print error on stderr, return -1 
*/
int read_request(int client_sockfd, char *buf, int *len)
{
    int bytes_received, bytes_total;
    bytes_total = 0;
    while (bytes_total < *len && ((bytes_received = recv(client_sockfd, buf + bytes_total, *len - bytes_total, 0)) > 0))
    {
        bytes_total += bytes_received;
        char *lf;
        if ((lf = memchr(buf, '\n', bytes_total)) != NULL)
        {
            if (*(lf - 1) == '\r') 
            {                      
                *len = bytes_total;
                return 0;
            }
        }
    }
    if (bytes_total >= *len)
    {
        fprintf(stderr, "Exceeded the maximum size limit\n");
        return -1;
    }
    else if (bytes_received == -1)
    {
        perror("server recv");
        return -1;
    }
    else 
    {
        fprintf(stderr, "Client closed connection before a request was read\n");
        return -1;
    }
}

/*
    Iterate over the buffer characters while current character isn't SP / LF
        Load character into word string
        Reallocate to size + 1 byte
    Terminate the word string with \0 at the lastly allocated memory.
    Return word
*/
char *read_input(char *buf, char **current)
{
    if (*current == NULL)
        *current = buf;
    size_t len = 0;
    char *word = malloc(sizeof(char));

    while (**current == ' ')
        (*current)++;
    if (**current == '\r' && **(current + 1) == '\n')
    {
        word = realloc(word, 3);
        word[0] = '\r';
        word[1] = '\n';
        word[3] = '\0';
        return word;
    }

    while (**current != ' ' && **current != '\r')
    {
        *(word + len) = **current;
        len++;
        (*current)++;
        word = realloc(word, len + 1);
    }
    *(word + len) = '\0';
    return word;
}

/*
    Serves the file named as string filename from root_dir. "Serves" in this function means
    filling up the buffer with the contents of the file, dynamically reallocating memory as needed.
*/
void serve(char *filename, int root_dir, char *buf, int *len)
{
    int filefd = openat(root_dir, filename, O_RDONLY);
    int bytes_total = 0;
    int bytes_read;
    int space_left = *len;
    int n = 2;

    while ((bytes_read = read(filefd, buf + bytes_total, space_left)) > 0)
    {
        bytes_total += bytes_read;
        space_left = *len - bytes_total;
        if (space_left == 0)
        {
            buf = realloc(buf, n * (*len));
            *len = n * (*len);
            space_left = *len - bytes_total;
            n++;    
        }
    }
    *len = bytes_total;
}