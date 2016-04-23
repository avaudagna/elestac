
#include "socketCommons.h"
#define LOCALHOST "127.0.0.1"

int main (int argc, char **argv) {
	puts("bye world");
	int serverSocket, port = 4400;
	int clientSocket, read_size = 0;
	char *messageBuffer = (char*) malloc(sizeof(char)*PACKAGE_SIZE);
	if(messageBuffer == NULL) {
		puts("===Error in messageBuffer malloc===");
		return (-1);
	}
	setServerSocket(&serverSocket, LOCALHOST, port);
	acceptConnection (&clientSocket, &serverSocket);

	//Receive a message from client
	while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) {
		if(strcmp(messageBuffer, "console")){
			puts ("Console is calling, let's handshake!");
			//If console is calling -> handshake
			write(clientSocket , "kernel" , strlen(messageBuffer));
			// and now I'm supposed to receive the AnSisOp program
			while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) {
				if(strcmp(messageBuffer,"#VamoACalmarno")){
					break;
				}else if(messageBuffer!=NULL){
					// recv ansisop code
					puts(messageBuffer);
				}
			}
		}else if(strcmp(messageBuffer, "existo")){ // CPU is alive!
			write(clientSocket , "so do I" , strlen(messageBuffer));
		}else{
			puts("Somebody is calling");
			write(clientSocket , "Identify your self" , strlen(messageBuffer));
		}
	}

	if(read_size == 0) {
		puts("Client disconnected");
		fflush(stdout);
	}else if(read_size == -1) {
		perror("recv failed");
	}
	close(clientSocket);
	close(serverSocket);
	return 0;
}
