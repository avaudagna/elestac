
#ifndef SOCKETCOMMONS_H
#define SOCKETCOMMONS_H

#define PACKAGE_SIZE 1024

#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen, malloc
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write

int getClientSocket (int* clientSocket, const char* address, const int port) ;
int setServerSocket (int* serverSocket, const char* address, const int port);
int acceptConnection(int serverSocket);

#endif
