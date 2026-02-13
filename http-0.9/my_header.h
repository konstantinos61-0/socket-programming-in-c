#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

#include <sys/socket.h> 
#include <sys/types.h>

#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>

#include <stdint.h>     
#include <unistd.h>
#include <fcntl.h>

#define MY_PORT "8080"
#define BACKLOG 5
#define BUFF_SIZE 4096

// Parser functions
void handle_connection(int client_sockfd, int root_dir);

// Helper functions associated directly with sockets
int send_all(int client_sockfd, char *buf, int *len);
int bind_to_port(int family, int socktype, const char *port);
void *get_sin_addr(struct sockaddr *p);

// Helper functions associated with request/response handling
int serve(char *filename, int root_dir, char *buf, int *len);
char *read_input(char *buf, char **current);
int read_request(int client_sockfd, char *buf, int *len);