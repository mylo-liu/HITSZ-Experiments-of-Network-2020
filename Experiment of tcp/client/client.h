#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <pthread.h>

#define MYPORT  5999
#define BUF_SIZE 1024
char* SERVER_IP = "192.168.193.129";
#define QUEUE   20
#define FILE_NAME_MAX_LEN 512

#endif