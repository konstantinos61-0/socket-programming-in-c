#include <stdio.h> // for stdio, including error printing
#include <stdlib.h>
#include <string.h> // for string handling, including error strings

#include <sys/socket.h> // for socket API, socketaddr structs
#include<sys/types.h>

#include <arpa/inet.h> // for Network byte order translations, IP address handling ?
#include <netinet/in.h> // IP addresses
#include <stdint.h> // for integer data types like uint16_t
#include <netdb.h>
#include <unistd.h>

#define MY_PORT "8080"
#define BACKLOG 5
#define BUFF_SIZE 4096


int send_all(int, char *, int *);
int bind_to_port(int family, int socktype, const char *port);
void *get_sin_addr(struct sockaddr *p);