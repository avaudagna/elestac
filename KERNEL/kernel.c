/* Kernel.c by pacevedo */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> /* posix for common syscalls */

#include "parser/metadata_program.h"
#include "socketCommons.h"
#define MAX_CLIENTS 100
#define KERNEL_IP "127.0.0.1"
#define LOCALHOST "127.0.0.1"
#define UMC_IP "127.0.0.1"

int UMC_PORT = 56793; /* defines que funcionaran como string */
int KERNEL_PORT = 54326; /* cuando alan cambie la funcion de la common */

int CallUMC(char * message);
void tratarSeniales(int);
int rmvClosedClients(int *clientList, int *clientsOnline);
int newClient(int serverSocket, int *clientSocket, int *clientsOnline);
char* hardcodeameUnPrograma();
int main (int argc, char **argv) {
	enum {PEDIR_MEMORIA, ENVIAR_PCB, OTRA_COSA} acciones;
	signal (SIGINT, tratarSeniales);
	int i;
	fd_set allSockets;

	t_metadata_program* newPCB;
	int serverSocket;
	int clientSocket[MAX_CLIENTS];
	int read_size = 0;
	int clientsOnline=0; /* CPUs and Consoles */
	int maxSocket=0;

	char *messageBuffer = (char*) malloc(sizeof(char)*PACKAGE_SIZE);
	if(messageBuffer == NULL) return (-1);

	char *fakePCB = (char*) malloc(sizeof(char)*PACKAGE_SIZE); // TODO Delete fakePCB
	// free() the malloc();
	puts("\n Initializing the system kernel. #VamoACalmarno \n");

	setServerSocket(&serverSocket, KERNEL_IP, KERNEL_PORT);

	while(1){
		maxSocket = rmvClosedClients (clientSocket, &clientsOnline); /* update clients online counter */
		if (maxSocket<serverSocket) maxSocket=serverSocket;
		FD_ZERO(&allSockets); /* Clear all sockets */
		FD_SET(serverSocket, &allSockets);
		/* Add all clients to the select() */
		for (i=0; i<clientsOnline; i++)	FD_SET (clientSocket[i], &allSockets);
		select (maxSocket+1, &allSockets, &allSockets, NULL, NULL);
		/* Check existing clients */
		for (i=0; i<clientsOnline; i++){
			if(FD_ISSET(clientSocket[i], &allSockets)){
				if((read_size = recv(clientSocket[i], messageBuffer, PACKAGE_SIZE, 0)) > 0){
					if(strcmp(messageBuffer, "console") == 0){
						puts ("A console is calling, let's handshake!");
						// clientSocket[i] is calling, launch a thread to attend it.
						write(clientSocket[i], "kernel" , PACKAGE_SIZE);
					}
/*
 *  The following WHILE should be replaced by the existing loop to be able to attend all clients at once.
 *  launch a new thread to manage this particular request.
 */
/*
						while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) {
							if((strcmp(messageBuffer,"#VamoACalmarno")) == 0 ){
								// Code transfer is finished, call UMC !
								puts("\nThis is a fake ansisop program (hardcoded)\n");
								char* programa=hardcodeameUnPrograma(); // create fake ANSISOP program
								puts(programa); // print it!
								newPCB = metadata_desde_literal(programa); // get metadata from the program
								printf("Cantidad de etiquetas: %i \n\n",newPCB->cantidad_de_etiquetas); // print something
								free(programa); // let it free
								//if(CallUMC()){ //	if(CallUMC(fakePCB)){
								//	puts("PBC successfully sent to UMC");
								//}
								acceptConnection (&clientSocket, &serverSocket);
								break;
							}else if(messageBuffer){
								// recv ansisop code
								puts("\n Data received from console: ");
								puts(messageBuffer);
								//strcat(fakePCB,messageBuffer);
								if(CallUMC(messageBuffer)){ //	if(CallUMC(fakePCB)){
									puts("PBC successfully sent to UMC");
								}
							}
						}
					}else if((strcmp(messageBuffer, "cpu")) == 0 ){ // CPU is alive!
						//write(clientSocket, fakePCB, strlen(fakePCB)); //send fake PCB
						write(clientSocket, messageBuffer, PACKAGE_SIZE); //send fake PCB
						while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) { // wait for CPU to finish
							if(messageBuffer!=NULL) puts(messageBuffer); // if it's "Elestac" we're happy.
							break;
						}
					}else{
						puts("Somebody is calling");
						write(clientSocket , "Identify your self" , strlen(messageBuffer));
						break;
					}
*/
		        }else{
		            /* Error in rcv, possible connection lost. Close the socket and delete the client. */
		        	printf("Client %d closed the connection. \n", i+1);
		        	fflush(stdout);
		        	clientSocket[i] = -1;
		        	close(clientSocket[i]);
		        	// should I use FD_CLR here?

		        	/*
		        	 *
		        	 * TODO
		        	 *  Acá tengo que ver si es la consola (que se mandó un ctrl+c o similar)
		        	 *  Si es la consola (la tengo identificada en un vector paralelo por ID),
		        	 *  Tengo que avisarle a CPU y UMC que ya fue, que cierre todo y nos vamo a casa.
		        	 *
		        	 */
		        }
		    }
		}
		/* Accept new connections */
		if (FD_ISSET(serverSocket, &allSockets)){
			// ALWAYS enters here and checks for new clients
			// Function will return 0 or -1 unless a new connection was created!
			int newClientID=newClient(serverSocket, &clientSocket, &clientsOnline);
			if (newClientID >=0){
				printf("The new client is %d",newClientID);
			}
			/*
			 * TODO
			 *
			 * Acá ya le envié su ID, espero arriba para hacer el handshake, clasificar al cliente (ver si es CPU o consola)
			 * y guardarme esa info en un vector paralelo por ID.
			 * Así después sé qué hacer con cada uno.
			 *
			 */
		}
	}
	free(messageBuffer);
	close(serverSocket);
	return 0;
}
int CallUMC(char * message){
	int clientUMC;
	//char* aPCB = PCB;
	//char aPCB[] = "KERNEL";
	char *clientMessage = NULL, *serverMessage = NULL;
    //Create socket
    if(getClientSocket(&clientUMC, UMC_IP, UMC_PORT)) {
    	return (-1);
    }
    clientMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
    if(clientMessage == NULL) return (-1);
    serverMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
    if(serverMessage == NULL) return (-1);
  //  while(1) {
      //  printf("Enter message : ");
     //   fgets(clientMessage, PACKAGE_SIZE, stdin);
     //   clientMessage[strlen(clientMessage) - 1] = '\0';

        //Send some data
        //if( send(clientUMC , aPCB , PACKAGE_SIZE , 0) < 0) {
        //    puts("Send failed");
        //    return 1;
        //}
    	//Send SAME data
	   if( send(clientUMC , "KERNEL" , PACKAGE_SIZE , 0) < 0) {
	       puts("Send failed");
	       return 1;
	   }
       if( recv(clientUMC , serverMessage , PACKAGE_SIZE , 0) < 0) {
           puts("recv failed");
           //break;
       }
	   if( send(clientUMC , message , PACKAGE_SIZE , 0) < 0) {
	       puts("Send failed");
	       return 1;
	   }
        //Receive a reply from the server
        if( recv(clientUMC , serverMessage , PACKAGE_SIZE , 0) < 0) {
            puts("recv failed");
            //break;
        }
        puts("UMC reply :");
        puts(serverMessage);
        clientMessage[0] = '\0';
        serverMessage[0] = '\0';
   //}
    close(clientUMC);
    return 1;
}
char* hardcodeameUnPrograma(){
	char *programita = NULL;
	char tempbuff[20]="";
	size_t programlen = 0;

	strcpy(tempbuff,"begin\n");
	programlen += strlen(tempbuff);
	programita = realloc(programita, programlen+1);
	strcat(programita, tempbuff);

    strcpy(tempbuff,"variables a, b\n");
	programlen += strlen(tempbuff);
	programita = realloc(programita, programlen+1);
	strcat(programita, tempbuff);

	strcpy(tempbuff,"a = 3\n");
	programlen += strlen(tempbuff);
	programita = realloc(programita, programlen+1);
	strcat(programita, tempbuff);

	strcpy(tempbuff,"b = 5\n");
	programlen += strlen(tempbuff);
	programita = realloc(programita, programlen+1);
	strcat(programita, tempbuff);

	strcpy(tempbuff,"a = b + 12\n");
	programlen += strlen(tempbuff);
	programita = realloc(programita, programlen+1);
	strcat(programita, tempbuff);

	strcpy(tempbuff,"end\n");
	programlen += strlen(tempbuff);
	programita = realloc(programita, programlen+1);
	strcat(programita, tempbuff);

	return programita;
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
	if((clientList == NULL) || ((*clientsOnline) == 0)) return 0;
	for(i=0; i<(*clientsOnline); i++){
		if(clientList[i] != -1){
			clientList[j] = clientList[i];
			j++;
		}
	}
	*clientsOnline = j; return clientList[j];
}
int newClient(int serverSocket, int *clientSocket, int *clientsOnline){
/* Accepts a new connection from a client.
 * Returns  0 if max clients have been reached.
 * Returns -1 if the connection failed.
 * Returns the socket descriptor ID if success was achieved.
 */
	if((*clientsOnline)+1>=MAX_CLIENTS) return 0;
	struct sockaddr_in clientConf;
	int c = sizeof(struct sockaddr_in);
	clientSocket[*clientsOnline] = accept(serverSocket, (struct sockaddr *)&clientConf, (socklen_t*)&c);
	if (clientSocket[*clientsOnline] < 0) {
	//	perror("Accept failed :(");
		return -1;
	}
	/* Server sends to the client the clientID and handshake will continue when returning to the select() */
	write(clientSocket[*clientsOnline] , (char *) clientsOnline , sizeof(int));
	printf ("Client number %d accepted! :)\n", *clientsOnline);
	(*clientsOnline)++;
	return ((*clientsOnline)-1);
}
