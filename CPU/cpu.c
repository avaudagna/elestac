#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <commons/log.h>
#include <socketCommons.h>
#include <parser.h>
#include "implementation_ansisop.h"

AnSISOP_funciones functions = {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= dereferenciar,
		.AnSISOP_asignar				= asignar,
		.AnSISOP_imprimir				= imprimir,
		.AnSISOP_imprimirTexto			= imprimirTexto,

};
AnSISOP_kernel kernel_functions = { };


int main(void) {

	int i;
	int cpuSocketServer, cpuClientSocket;
	int umcSocketClient, kernelSocketClient;
	char kernelHandShake[] = "Existo";
	t_log logFile = {
		open("cpuLog.log"),
		true,
		LOG_LEVEL_INFO,
		"CPU",
		1234
	};
	char* kernelMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
	if(kernelMessage == NULL) {
		puts("===Error in kernelMessage malloc===");
		return (-1);
	}
	char* umcMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
	if(umcMessage  == NULL) {
		puts("===Error in umcMessage  malloc===");
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
		if( recv(kernelSocketClient , kernelMessage , PACKAGE_SIZE , 0) < 0) {
				puts("recv failed");
				break;
		}
		puts("Server reply :");
		puts(kernelMessage);

		//Wait for PCB from the Kernel
		if( recv(kernelSocketClient , kernelMessage , PACKAGE_SIZE , 0) < 0) {
				puts("recv failed");
				break;
		}
		puts("Server reply :");
		puts(kernelMessage);

		//Send message to the UMC
		if( send(umcSocketClient , "umc"  , strlen("umc") , 0) < 0) {
				puts("Send failed");
				return 1;
		}
		//Wait for PCB from the Kernel
		if( recv(umcSocketClient , umcMessage , PACKAGE_SIZE , 0) < 0) {
				puts("recv failed");
				break;
		}
		puts("Server reply :");
		puts(umcMessage);

		log_info(logFile, "Analizando linea");
		analizadorLinea(strdup(umcMessage), &functions, &kernel_functions);

		//Supongamos que hago algo
		for(i=0; i<100000; i++);

		//Aca es donde uso el parser con ese mensaje para generar las lineas a ejecutar
		//de lo que me devuelva el parser, devuelvo el resultado para imprimir al kernel

		//Send message to the Kernel
		if( send(kernelSocketClient , "exit0"  , strlen("exit0") , 0) < 0) {
				puts("Send failed");
				return 1;
		}
	}
	return 0;
}
