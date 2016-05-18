/* Kernel.c by pacevedo */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
//#include <unistd.h> /* posix for common syscalls */
#include <commons/string.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include "socketCommons.h"

#define MAX_CLIENTS 100
#define KERNEL_IP "127.0.0.1"
#define LOCALHOST "127.0.0.1"
#define UMC_IP "127.0.0.1"
#define PROTOCOL_SIZE 2

int UMC_PORT = 56793; /* defines que funcionaran como string */
int KERNEL_PORT = 54326; /* cuando alan cambie la funcion de la common */

int RequestPages2UMC(char * codigo,t_metadata_program* metadata);
void tratarSeniales(int);
int rmvClosedClients(int *clientList, int *clientsOnline);
int newClient(int serverSocket, int *clientSocket, int clientsOnline);

int main (int argc, char **argv) {
	//enum {PEDIR_MEMORIA, ENVIAR_PCB, OTRA_COSA} acciones;
	signal (SIGINT, tratarSeniales);
	int i;
	fd_set allSockets;

	int serverSocket;
	int clientSocket[MAX_CLIENTS]={0};
	int read_size = 0;
	int clientsOnline=0; /* CPUs and Consoles */
	int maxSocket=0;

	char *messageBuffer = (char*) malloc(sizeof(char)*PACKAGE_SIZE);
	if(messageBuffer == NULL) return (-1);
	setServerSocket(&serverSocket, KERNEL_IP, KERNEL_PORT); // Create the server
	while(1){
		sleep(1);
		maxSocket = rmvClosedClients (clientSocket, &clientsOnline); /* update clients online counter */
		if(maxSocket<serverSocket) maxSocket=serverSocket;
		FD_ZERO(&allSockets); /* Clear all sockets */
		FD_SET(serverSocket, &allSockets);
		/* Add all clients to the select() */
		for (i=0; i<clientsOnline; i++)	FD_SET(clientSocket[i], &allSockets);
		if(select (maxSocket+1, &allSockets, NULL, NULL, NULL)>0){
			puts("Sali del select con algo interesante.");
		}
		/* Check existing clients */
		for (i=0; i<clientsOnline; i++){
			if(FD_ISSET(clientSocket[i], &allSockets)){
				if((read_size = recv(clientSocket[i], messageBuffer, 2, 0)) > 0){
					if(strcmp(messageBuffer, "C0") == 0){
						//CPU connection
						read_size = recv(clientSocket[i], messageBuffer, PACKAGE_SIZE, 0);
						printf("CPU says:%s\n",messageBuffer);
					}else if((strcmp(messageBuffer, "C1")) == 0 ){
						// CPU msg 1
						//write(clientSocket, fakePCB, strlen(fakePCB)); //send fake PCB
						write(clientSocket, "holi", 4); //send fake PCB
						while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) {
							// wait for CPU to finish
							if(messageBuffer!=NULL) puts(messageBuffer);
							break;
						}
					}else{
							puts("Caso no contemplado. Caimos en default (?");
							printf("Los clientes andan diciendo:%s\n",messageBuffer);
					}
				}else{
		            /* Error in rcv, possible connection lost.
		             * Close the socket and delete the client. */
		        	printf("Client %d closed the connection. \n", i+1);
		        	fflush(stdout);
		        	//FD_CLR(clientSocket[i], &allSockets);
		        	clientSocket[i] = -1;
		        	close(clientSocket[i]);
	/*
	 * TODO
	 *  Acá tengo que ver si es la consola (que se mandó un ctrl+c o similar)
	 *  Si es la consola (la tengo identificada en un vector paralelo por ID),
	 *  Tengo que avisarle a CPU y UMC que ya fue, que cierre todo y nos vamo a casa.*/
		        }
		    }
		}
		/* Accept new connections and handshake */
		if (FD_ISSET(serverSocket, &allSockets)){
			// ALWAYS enters here and checks for new clients
			// Function will return 0 or -1 unless a new connection was created!
			printf("Clientes antes de aceptar: %d \n", clientsOnline);
			int aceptado=newClient(serverSocket, clientSocket, clientsOnline);
			if(aceptado>0){
				clientsOnline=aceptado;
				char clientID[2];
				sprintf(clientID, "%02d\n", (int) clientSocket[clientsOnline-1]);
				printf("The new client is %s\n",clientID);
				if((read_size = recv(clientSocket[clientsOnline-1], messageBuffer, 2, 0)) > 0){
					if(strcmp(messageBuffer, "T0") == 0){
						puts ("A console is calling, let's handshake!");
						// TODO Add the console to the list of console clients.
						/* Server sends to the client the clientID */
						write(clientSocket[clientsOnline-1],clientID,2);

						char ansisopLen[4];
						recv(clientSocket[clientsOnline-1], messageBuffer, 4, 0);
						recv(clientSocket[clientsOnline-1], &ansisopLen, 4, 0);
						//TODO Check if ansisopLen is taking the 4 digits of code length.
						char *codigo = malloc(atoi(ansisopLen));
						recv(clientSocket[clientsOnline-1], codigo, ansisopLen, 0);
						printf("El codigo ansisop recibido es:\n%s\n",codigo);
						t_metadata_program* newPCB = metadata_desde_literal(codigo);
						//printf("Cantidad de etiquetas: %i \n\n",newPCB->cantidad_de_etiquetas);
						if(RequestPages2UMC(codigo,newPCB)){
							puts("Ask UMC for pages to allocate PCB");
							//create PCB and submit to UMC for storage
							//serialize newPCB and submit
						}else{
							puts("Sorry, no hay espacio para tu programita.");
						}
						free(codigo); // let it free
					}
				}
			}
			printf("Clientes despues de aceptar: %d\n", clientsOnline);
		}
	}
	free(messageBuffer);
	close(serverSocket);
	return 0;
}
int RequestPages2UMC(char * codigo,t_metadata_program* metadata){
	int clientUMC;
    //Create socket
    if(getClientSocket(&clientUMC, UMC_IP, UMC_PORT)) {
    	return (-1);
    }

    char *msg = NULL;
    msg = (char*) calloc(sizeof(char),PACKAGE_SIZE);

	   if( send(clientUMC , "hay espacio?" , PACKAGE_SIZE , 0) < 0) {
	       puts("Send failed");
	       return 1;
	   }
       if( recv(clientUMC , msg , 1, 0) < 0) {
           puts("recv failed");
       }
	   if( send(clientUMC , codigo , PACKAGE_SIZE , 0) < 0) {
	       puts("Send failed");
	       return 1;
	   }
       if( recv(clientUMC , msg , PACKAGE_SIZE , 0) < 0) {
            puts("recv failed");
        }
        puts("UMC reply :");
        puts(msg);
    close(clientUMC);
    return 1;
}
void tratarSeniales(int senial){
	printf ("Tratando seniales\n");
	printf("\nSenial: %d\n",senial);
	switch (senial){
		case SIGINT:
			// Detecta Ctrl+C y evita el cierre.
			printf("Esto acabará con el sistema. Presione Ctrl+C una vez más para confirmar.\n\n");
			signal (SIGINT, SIG_DFL); // solo controlo una vez.
			break;
		case SIGPIPE:
			// Trato de escribir en un socket que cerro la conexion.
			printf("La consola o el CPU con el que estabas hablando se murió. Llamá al 911.\n\n");
			signal (SIGPIPE, tratarSeniales);
			break;
	}
}
int rmvClosedClients(int *clientList, int *clientsOnline){
/* Removes client sockets which closed the connection.
 * Removes "-1" from the list, updates the clientsOnline.
 * Returns the ID of the last socket descriptor in the list.
 */
	int i;int j=0;
	if(*clientsOnline == 0){ return 0; }
	for(i=0; i<(*clientsOnline); i++){
		if(clientList[i] != -1 && clientList[i]>0){
			clientList[j] = clientList[i];
			j++;
		}
	}
	*clientsOnline = j; return clientList[j];
}
int newClient(int serverSocket, int *clientSocket, int clientsOnline){
/* Accepts a new connection from a client.
 * Returns  0 if max clients have been reached.
 * Returns -1 if the connection failed.
 * Returns the socket descriptor ID if success was achieved.
 */
//	if(clientsOnline>=MAX_CLIENTS) return 0;
//	struct sockaddr_in clientConf;
//	int c = sizeof(struct sockaddr_in);
	//clientSocket[clientsOnline] = accept(serverSocket, (struct sockaddr *)&clientConf, (socklen_t*)&c);
	clientSocket[clientsOnline]=acceptConnection((int) clientSocket[clientsOnline],serverSocket);
	if (clientSocket[clientsOnline] < 0) {
		perror("Accept failed :(");
		return -1;
	}else{
		printf ("Client number %d accepted! :)\n", clientSocket[clientsOnline]);
	}
	return (++clientsOnline);
}
