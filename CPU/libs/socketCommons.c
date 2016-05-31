#include "socketCommons.h"

int getClientSocket(int* clientSocket, const char* address, const int port) {
	struct sockaddr_in server;
	*clientSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (*clientSocket == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");

	server.sin_addr.s_addr = inet_addr(address);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server
	if (connect(*clientSocket , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		return (-1);
	}
	puts("Connected to Server\n");
	return 0;
}

int setServerSocket(int* serverSocket, const char* address, const int port) {
	struct sockaddr_in serverConf;

	//Create socket
	*serverSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (*serverSocket == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	serverConf.sin_family = AF_INET;
	serverConf.sin_addr.s_addr = inet_addr(address);
	serverConf.sin_port = htons(port);

	//Bind
	if( bind(*serverSocket,(struct sockaddr *)&serverConf , sizeof(serverConf)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return (-1);
	}
	puts("bind done");

	//Listen
	listen(*serverSocket , 3);
	return 0;
}

int acceptConnection (int *clientSocket, int* serverSocket) {
	int c;
	struct sockaddr_in clientConf;

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	*clientSocket = accept(*serverSocket, (struct sockaddr *)&clientConf, (socklen_t*)&c);
	if (*clientSocket < 0) {
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");
	return 0;
}
