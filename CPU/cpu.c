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
	int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];
	PCB incoming_pcb;

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	//Accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");

	//Recibo el mensaje del nucleo (el PCB)
	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 )
	{
		incoming_pcb = (PCB) client_message;
		//Incrementar el PC del PCB
		//Utilizar el indice de codigo para solicitar a la UMC la proxima sentencia a ejecutar
		//Al recibirla parsearla
		//Ejecutar operaciones requeridas
		//Actualizar valores de programa en UMC
		//Actualizar PC en PCB
		//Notificar a nucleo que concluyo un quantum
		//Loop de ejecucion, le pido cada sentencia a la UMC
	}

	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}

	return 0;
}

