/*
gcc -o console socketCommons/socketCommons.c test_pablo_console.c
*/
#include<stdlib.h> //exit(1);
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include "socketCommons/socketCommons.h"
#include <signal.h>

#define KERNEL_ADDR "127.0.0.1"
#define KERNEL_PORT 5007

void tratarSeniales(int);
int kernelSocketClient;
int main(int argc , char *argv[]){
	signal (SIGINT, tratarSeniales);
	char kernel_reply[2000];
	if(argc != 2) {
		puts("Usage: ./console facil.ansisop");
		return -1;
    }
	getClientSocket(&kernelSocketClient, KERNEL_ADDR, KERNEL_PORT);

	FILE * fp;
	fp = fopen (argv[1], "r");
	fseek(fp, SEEK_SET, 0);

	fseek(fp, 0L, SEEK_END);
	long int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	//Despues haces malloc con sz
	char* prog = (char*) malloc(sz);

	fread(prog, sz+1, 1, fp);
	fclose(fp);

	int sizeMsj = strlen("0")+4+(int) sz;
	char* mensaje = (char*) malloc(sizeMsj);

    char buffer[4];
	sprintf(buffer, "%04d", (int) sz);

	strcpy(mensaje, "0");
	strcat(mensaje, buffer);
	strcat(mensaje, prog);
	send(kernelSocketClient, mensaje, sizeMsj, 0);
	printf("Sent: %s \n", mensaje);
	puts("Ahora espero");
	recv(kernelSocketClient , buffer, 4, 0);
	printf("Received: %s \n", buffer);
	//send(kernelSocketClient, "2", 1, 0);
	while(1);
    close(kernelSocketClient);
    puts("Socket Closed");
	return 0;
}

void tratarSeniales(int senial){
	printf ("Tratando seniales\n");
	printf("\nSenial: %d\n",senial);
	switch (senial){
		case SIGINT:
			// Detecta Ctrl+C y evita el cierre.
			//printf("Esto acabará con el sistema. Presione Ctrl+C una vez más para confirmar.\n\n");
			//signal (SIGINT, SIG_DFL); // solo controlo una vez.
			printf("Program execution cancelled.\n\n");
			//send(kernelSocketClient, "1", 1, 0);
            //close(kernelSocketClient);
			exit(1);
			break;
	}
}