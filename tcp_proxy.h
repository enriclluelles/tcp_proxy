#ifndef __TCP_PROXY__H_
#define __TCP_PROXY__H_
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include "log.h"

#define MAX_MSG_SIZE 2048
#define MAX_PORTS 10

struct conn;
struct listener;
void print_usage(char *name) __attribute__ ((noreturn));
void print_address(struct sockaddr_in* p);
struct sockaddr_in* addr_from_host(char host[]);
struct sockaddr_in* addr_from_ip(char ip[]);
void sock_setreuse(int fd, int reuse);
void sock_settimeout(int sockfd, int timeout);
int listen_to_port(int port);
void *sender(void *args);
void *receiver(void *args);
void *client_handler(void *args);
void accept_connections(int socketfd, struct sockaddr_in *address);
void *listen_conn(void *args);
#endif
