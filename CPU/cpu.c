#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "CPU.h"

#define IP "127.0.0.1"
#define PUERTO "1234"
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

int main(void) {
	char client_message[PACKAGESIZE], codeBlock[PACKAGESIZE];
	int kernelClient, umcClient;
	PCB incoming_pcb;
	char message[1000];
	char* nextLine = (char*) malloc(NEXT_LINE_SIZE), parsedLines;
	if (nextLine == NULL)
	{
		printf("No hay espacio suficiente para nextLine");
	}
	//Create socket
	getKernelClient(&kernelClient);
	getUmcClient(&umcClient);
	//CPU loop
	//Recv Kernell message (PCB) BLOCKING

	while (read_size = recv(kernelClient , incoming_pcb , PACKAGESIZE , 0) > 0) {
		incoming_pcb = (PCB) client_message;
		incrementPC(&incoming_pcb); //Incrementar el PC del PCB

		do(getNextLine(umcClient, incoming_pcb.codeIndex, &nextLine)) {//Utilizar el indice de codigo para solicitar a la UMC la proxima sentencia a ejecutar
			parseNextLine(nextLine, &parsedLines);//Al recibirla parsearla
			executeParsedLines(parsedLines);//Ejecutar operaciones requeridas
			//??? //Actualizar valores de programa en UMC
			incrementPC(&incoming_pcb);//Actualizar PC en PCB
			notifyKernelQuantumFinished();//Notificar a nucleo que concluyo un quantum
			//Loop de ejecucion, le pido cada sentencia a la UMC
		} while (nextLine != NULL)
	}
	if(read_size == 0) {
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1) {
		perror("recv failed");
	}

	close(kernelClient);
	close(umcClient);
	return 0;
}

void incrementPC(PCB* incoming_pcb) {
	incoming_pcb->PC++;
}

void getParsedInstructions(char* nextLine, char** parsedLines){
	*parsedLines = parser.parse(nextLine);
}
void executeParsedLines (char* parsedLines) {
	//for instruction in line execute instruction
}

void getNextLine(int umcClient,int codeIndex, char** nextLine) {
	send(umcClient, codeIndex, sizeof(codeIndex), 0); //Sending index
	if ((read_size = recv(umcClient, *nextLine, NEXT_LINE_SIZE , 0)) > 0) {
		puts("Next Line received");
	}
	if(read_size == 0) {
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1) {
		perror("recv failed");
	}
}

void getKernelClient(int* sock) {
	struct sockaddr_in server;
	*sock = socket(AF_INET , SOCK_STREAM , 0);
	if (*sock == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");

	server.sin_addr.s_addr = inet_addr(KERNEL_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons(KERNEL_PORT);

	//Connect to remote server
	if (connect(*sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}
	puts("Connected to Kernel\n");
}
void getUmcClient(int* sock) {
	struct sockaddr_in server;
	*sock = socket(AF_INET , SOCK_STREAM , 0);
	if (*sock == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");

	server.sin_addr.s_addr = inet_addr(UMC_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons( UMC_PORT );

	//Connect to remote server
	if (connect(*sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}
	puts("Connected to UMC\n");
}
