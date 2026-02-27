#include "server.h"
/*
    Creates a socket of family and socktype and binds it at a local port. Returns the socket 
    descriptor. On fatal error, it prints the error message on stderr and returns -1
*/
#define SUPP_RESP_HEADS 3
char *supported_response_headers[] = {
    "content-type", "content-length", "connection" // In a chosen canonical form of all lowercase, for use in comparisons etc.
};

int bind_to_port(int family, int socktype, const char *port)
{
    int status, server_sockfd, yes;
    struct addrinfo hints, *p, *res;
    yes = 1;
    char ip[INET6_ADDRSTRLEN];

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
        printf("Listening for connections on port %s...\n", port);
        if (p->ai_family == AF_INET)
        {
            inet_ntop(AF_INET, get_sin_addr(p->ai_addr), ip, INET6_ADDRSTRLEN);
            printf("Available in http://%s:%s/\n\n", ip,port);
        }
        else
        {
            inet_ntop(AF_INET6, get_sin_addr(p->ai_addr), ip, INET6_ADDRSTRLEN);
            printf("Available in http://%s:%s/\n\n", ip,port);
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
    int bytes_total = 0, bytes_sent;
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
        3. If CRLF CRLF is found in the buffer
            4. Set *len to the total bytes and return 0;
    5. If buffer is full, print error on stderr and return -1
    6. If recv caused an error, print error on stderr, return -1
    7. Else if client closed connection, print error on stderr, return -1 
*/
int read_request(int client_sockfd, char **buf, int *len)
{
    int bytes_received, bytes_total;
    int n = 2; // Buffer Reallocation factor, startinf at 2, meaning first reallocation happens with 2 * BUFF_SIZE 
    bytes_total = 0;
    while ((bytes_received = recv(client_sockfd, *buf + bytes_total, *len - bytes_total, 0)) > 0)
    {
        bytes_total += bytes_received;
        char *lf;
        if ((lf = memmem(*buf, bytes_total, "\r\n\r\n", strlen("\r\n\r\n"))) != NULL)
        {                    
            *len = bytes_total;
            return 0;
        }
        if (bytes_total >= *len)
        {
            // Reallocate more memory.
            char *tmp = realloc(*buf, n * BUFF_SIZE * sizeof(char));
            if (tmp == NULL)
            {
                return -1;
            }
            *buf = tmp;
            *len = n * BUFF_SIZE * sizeof(char);
            n++;
        }
    }
    if (bytes_received == -1)
    {
        perror("server recv");
        return -1;
    }
    else
    {
        fprintf(stderr, "Client closed connection before a request was read\n");
        return -2;
    }
}
// Inserts header_node n into the top of the header_node stack (linked list)
// On success, returns the pointer to the first list item (pointed to by list variable)
void push_node(header_node **list, header_node *n)
{
    header_node *tmp = *list; // Save the 1st item (where list points) in a tmp var
    *list = n; // Make list point to the new header node n
    n->next = tmp; // Make n point to the tmp to stitch the list back in the correct way   
}

// Prints the linked list of header nodes
void print_list(header_node *list)
{
    for (header_node *p = list; p != NULL; p = p->next)
        printf("%s: %s\n", p->header_field, p->header_value);
}

// Frees the linked list of header nodes
void free_list(header_node *list)
{
    header_node *ptr = list;
    while (ptr != NULL)
    {
        header_node *next = ptr->next;
        free(ptr);
        ptr = next;
    }
}

// Fills list with supported, valid response headers. Systematic Response header handling
void fill_response_headers(int filefd, char *filename, header_node **list, int *msg_len)
{
    for (int i = 0; i < SUPP_RESP_HEADS; i++)
    {
        if (!strcmp(supported_response_headers[i], "content-type")) 
        {
            header_node *n = malloc(sizeof(header_node));
            strcpy(n->header_field, "Content-Type");
            strcpy(n->header_value, mime_type(filename));
            push_node(list, n);
        }
        else if (!strcmp(supported_response_headers[i], "connection"))
        {
            header_node *n = malloc(sizeof(header_node));
            strcpy(n->header_field, "Connection");
            strcpy(n->header_value, "close");
            push_node(list, n);
        }
        else if (!strcmp(supported_response_headers[i], "content-length"))
        {
            struct stat statbuf;
            if (fstat(filefd, &statbuf) == -1)
                continue;
            header_node *n = malloc(sizeof(header_node));
            strcpy(n->header_field, "Content-Length");
            // Find length
            char length_s[10];
            long int length = (msg_len) ? (statbuf.st_size + *msg_len) : statbuf.st_size;
            sprintf(length_s, "%li", length);
            strcpy(n->header_value, length_s);
            push_node(list, n);
        }
    }


}


// Returns the MIME/type of filename
char *mime_type(char *filename)
{
    int ext_len;
    char *ext, *dot;
    if ((dot = strstr(filename, ".")) == NULL)
        return "application/octet-stream";
    ext= dot + 1;
    ext_len = strlen(ext);
    if (ext_len == 0)
        return "application/octet-stream";
    char *lowerc_ext = malloc (ext_len + 1);
    for (int i = 0; i < ext_len + 1; i++)
    {
        *(lowerc_ext + i) = tolower(*(ext + i));
    }
    if (!strcmp(lowerc_ext, "html"))
    {
        free(lowerc_ext);
        return "text/html";
    }
    else if (!strcmp(lowerc_ext, "jpeg") || !strcmp(lowerc_ext, "jpg"))
    {
        free(lowerc_ext);
        return "image/jpeg";
    }
    else if (!strcmp(lowerc_ext, "png"))
    {
        free(lowerc_ext);
        return "image/png";
    }
    else if (!strcmp(lowerc_ext, "mp4"))
    {
        free(lowerc_ext);
        return "video/mp4";
    }
    else if (!strcmp(lowerc_ext, "mp3"))
    {
        free(lowerc_ext);
        return "audio/mpeg";
    }
    else if (!strcmp(lowerc_ext, "pdf"))
    {
        free(lowerc_ext);
        return "application/pdf";
    }
    else if (!strcmp(lowerc_ext, "css"))
    {
        free(lowerc_ext);
        return "text/css";
    }
    else
    {
        free(lowerc_ext);
        return "application/octet-stream";
    }
}

/*
    Sends the filefd's contents to the client socket as well as all response headers beforehand.
    after inserting msg into the  <p id="msg"> tag of the file. 
    In the function's use, filefd is passed by default as the error.html template in the templates folder,
    Returns 0 on success, -1 on error.
*/  
int serve_error_template(int client_sockfd, int filefd, char *msg, char *filename)
{
    /*
        1. Read until you find the <p id="msg"> memory chunck inside error.html, sending everything you read into the socket.
        2. Save the amount of bytes read until the p tag
        2. Send the msg 
        3. Set offset of filefd to the saved value
        4. Send the rest of the file to the socket (the rest of the original file, closing the <p> tag on the way)
    */
    int msg_len = strlen(msg), offset = 0, bytes_read;
    char *tag = "<p id=\"msg\"", *p = NULL;
    char buf[BUFF_SIZE * sizeof(char)];
    header_node *res_h_list = NULL;

    // Send response headers
    fill_response_headers(filefd, filename, &res_h_list, &msg_len); // Fill and send response headers 
    // Loop into linked list and send each
    for (header_node *p = res_h_list; p != NULL; p = p->next)
    {
        char resp_header[MAX_HEADER_FIELD + MAX_HEADER_VALUE + 5]; // +4 for the 4 extra chars from field, value. +1 for '\0'
        sprintf(resp_header,"%s: %s\r\n", p->header_field, p->header_value);
        int rhlen = strlen(resp_header);
        send_all(client_sockfd, resp_header, &rhlen);
    }
    char *crlf = "\r\n";
    int crlf_len = strlen(crlf);
    send_all(client_sockfd, crlf, &crlf_len);

    while ((bytes_read = read(filefd, buf, BUFF_SIZE * sizeof(char))) > 0)
    {
        if ((p = memmem(buf, bytes_read, tag, strlen(tag))) != NULL)
        {
            int offset_inc = (p - buf + 1) + strlen(tag);
            offset = offset + offset_inc; // offset equals the amount of bytes read before reaching the end of the opening tag (last char included).
            send_all(client_sockfd, buf, &offset_inc);
            break;
        }
        else
        {
            offset += bytes_read;
            send_all(client_sockfd, buf, &bytes_read);
        }

    }
    if (bytes_read == -1)
        return -1;
    
    send_all(client_sockfd, msg, &msg_len);

    lseek(filefd, offset, SEEK_SET);
    while ((bytes_read = read(filefd, buf, BUFF_SIZE * sizeof(char))) > 0)
    {
        send_all(client_sockfd, buf, &bytes_read);
    }
    if (bytes_read == -1)
        return -1;

    return 0;
}