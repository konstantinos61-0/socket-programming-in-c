#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

#include <sys/socket.h> 
#include <sys/types.h>

#include <arpa/inet.h> 
#include <netdb.h>
#include <netinet/in.h> 
#include <stdint.h>     
#include <unistd.h>

#define MY_PORT "8080"
#define BACKLOG 5
#define BUFF_SIZE 4096

int send_all(int, char *, int *);
int bind_to_port(int family, int socktype, const char *port);
void *get_sin_addr(struct sockaddr *p);