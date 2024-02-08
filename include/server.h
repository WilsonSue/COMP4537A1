#ifndef SERVER_H
#define SERVER_H

#include "../include/db.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT_NUMBER 8080
#define BASE_TEN 10
#define FOURTEEN 14
#define MAX_HTTP_METHOD_LEN 16
#define SEVENTEEN 17
#define TWENTY_THREE 23
#define TWENTY_FOUR 24
#define MAX_PATH 1024
#define MAX_HTTP_HEADER_LEN 1024
#define MAX_FULL_PATH_LEN 2048

int  initialize_server_socket(const char *address, uint16_t port);
int  accept_connection(int server_socket);
void start_server(const char *address, uint16_t port, const char *webroot);

#endif    // SERVER_H
