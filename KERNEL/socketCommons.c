#include "socketCommons.h"

int getClientSocket(int* clientSocket, const char* address, const int port) {
	struct sockaddr_in server;
	*clientSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (*clientSocket == -1) printf("Could not create socket.\n");
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if(connect(*clientSocket , (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("Connect failed");
		return (-1);
	}
	printf("\n .:: Connected to server in %s:%d ::.\n", address, port);
	return 0;
}

int setServerSocket(int* serverSocket, const char* address, const int port) {
	struct sockaddr_in serverConf;
	*serverSocket = socket(AF_INET, SOCK_STREAM , 0);
	serverConf.sin_family = AF_INET;
	serverConf.sin_addr.s_addr = inet_addr(address);
	serverConf.sin_port = htons(port);
	if( bind(*serverSocket,(struct sockaddr *)&serverConf, sizeof(serverConf)) < 0) {
		perror("Bind failed");
		return (-1);
	}
	listen(*serverSocket , 3); // 3 is my queue of clients
	return 0;
}

int acceptConnection(int serverSocket) {
	struct sockaddr_in clientConf;
	int newClient; //can be CPU or Console
	int c = sizeof(struct sockaddr_in);
	newClient = accept(serverSocket, (struct sockaddr *)&clientConf, (socklen_t*)&c);
	if (newClient < 0) {
		perror("Accept failed");
		return (-1);
	}
	return newClient;
}
