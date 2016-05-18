#include "socketCommons.h"

int getClientSocket(int* clientSocket, const char* address, const int port) {
	struct sockaddr_in server;
	*clientSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (*clientSocket == -1) {
		printf("Could not create socket.\n");
	}
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server
	if (connect(*clientSocket , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("Connect failed.\n");
		return (-1);
	}
	puts("Connected to server\n");
	return 0;
}

int setServerSocket(int* serverSocket, const char* address, const int port) {
	struct sockaddr_in serverConf;
	*serverSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (*serverSocket == -1) puts("Could not create socket.");
	puts("Socket created");

	//Prepare the sockaddr_in structure
	serverConf.sin_family = AF_INET;
	serverConf.sin_addr.s_addr = inet_addr(address);
	serverConf.sin_port = htons(port);

	if( bind(*serverSocket,(struct sockaddr *)&serverConf , sizeof(serverConf)) < 0) {
		perror("Bind failed.");
		return (-1);
	}
	if (listen(*serverSocket , 3)==0){
		// where 3 is the queue of pending connections we will accept
		//puts("Estoy listeneando OK");
	}
	return 0;
}

int acceptConnection (int clientSocket, int serverSocket) {
	int c;
	struct sockaddr_in clientConf;
	c = sizeof(struct sockaddr_in);
	clientSocket = accept(serverSocket, (struct sockaddr *)&clientConf, (socklen_t*)&c);
	if (clientSocket < 0) {
		perror("Accept failed.");
		return 1;
	}
	puts("Connection accepted.");
	return clientSocket;
}
