#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "socketCommons/socketCommons.h"


int main(void) {

	int cpuSocketServer, cpuClientSocket;
	int umcSocketClient, kernelSocketClient;
	char kernelHandShake[] = "Existo";
	char* serverMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
	if(serverMessage == NULL) {
		puts("===Error in messageBuffer malloc===");
		return (-1);
	}

	//Create CPU server
	setServerSocket(&cpuSocketServer, KERNEL_ADDR, KERNEL_PORT);
	//Create CPU client
	acceptConnection (&cpuClientSocket, &cpuSocketServer);

	//Crete UMC client
	getClientSocket(&umcSocketClient, UMC_ADDR , UMC_PORT);
	//Crete Kernel client
	getClientSocket(&kernelSocketClient, KERNEL_ADDR , KERNEL_PORT);

	//keep communicating with server
	while(1) {
		//Send a handshake to the kernel
		if( send(kernelSocketClient , kernelHandShake , strlen(kernelHandShake) , 0) < 0) {
				puts("Send failed");
				return 1;
		}
		//Receive a reply of the handshake from the Kernel
		if( recv(kernelSocketClient , serverMessage , PACKAGE_SIZE , 0) < 0) {
				puts("recv failed");
				break;
		}
		puts("Server reply :");
		puts(serverMessage);

		//Wait for PCB from the Kernel
		if( recv(kernelSocketClient , serverMessage , PACKAGE_SIZE , 0) < 0) {
				puts("recv failed");
				break;
		}
		puts("Server reply :");
		puts(serverMessage);

		//Send PCB to the UMC
		if( send(umcSocketClient , serverMessage , strlen(serverMessage) , 0) < 0) {
				puts("Send failed");
				return 1;
		}

		//Wait for all this pain to end
		if( recv(kernelSocketClient , serverMessage , PACKAGE_SIZE , 0) < 0) {
				puts("recv failed");
				break;
		}
	}
	return 0;
}
