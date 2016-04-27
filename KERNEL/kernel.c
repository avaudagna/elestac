/* Kernel.c by pacevedo */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser/metadata_program.h"
#include "socketCommons.h"
#define KERNEL_IP "127.0.0.1"
#define LOCALHOST "127.0.0.1"
#define UMC_IP "127.0.0.1"

int UMC_PORT = 6750; // defines que funcionaran como string
int KERNEL_PORT = 60001; // cuando alan cambie la funcion de la common

int CallUMC();
char* hardcodeameUnPrograma();
int main (int argc, char **argv) {
	t_metadata_program* casiPCB;
	int serverSocket;
	int clientSocket, read_size = 0;
	char *messageBuffer = (char*) malloc(sizeof(char)*PACKAGE_SIZE);
	char *fakePCB = (char*) malloc(sizeof(char)*PACKAGE_SIZE);
	if(fakePCB == NULL){puts("Error in fakePCB"); return (-1);}else{strcpy(fakePCB,"");} // TODO Delete
	if(messageBuffer == NULL) {
		puts("===Error in messageBuffer malloc===");
		return (-1);
	}
	puts("\n Initializing the system kernel. #VamoACalmarno \n");
	char* programa=hardcodeameUnPrograma(); // create fake ANSISOP program
	puts(programa); // print it!
	casiPCB = metadata_desde_literal(programa);
	printf("Tengo algunas etiquetas: %i \n\n",casiPCB->cantidad_de_etiquetas);
	free(programa);
	setServerSocket(&serverSocket, KERNEL_IP, KERNEL_PORT);
	acceptConnection (&clientSocket, &serverSocket);
	//Receive a message from console
	while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) {
		if(strcmp(messageBuffer, "console") == 0){
			puts ("Console is calling, let's handshake!");
			//If console is calling -> handshake
			write(clientSocket , "kernel" , 7);
			// and now I'm supposed to receive the AnSisOp program
			while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) {
				if((strcmp(messageBuffer,"#VamoACalmarno")) == 0 ){
					// Code transfer is finished, call UMC !
					puts("\nThis is a fake ansisop program (hardcoded)\n");

				//	if(CallUMC(fakePCB)){
					if(CallUMC()){
						puts("PBC successfully sent to UMC");
					}
					break;
				}else if(messageBuffer!=NULL){
					// recv ansisop code
					puts(messageBuffer);
					strcat(fakePCB,messageBuffer);
				}
			}
		}else if((strcmp(messageBuffer, "cpu")) == 0 ){ // CPU is alive!
			write(clientSocket, fakePCB, strlen(fakePCB)); //send fake PCB
			while( (read_size = recv(clientSocket , messageBuffer , PACKAGE_SIZE , 0)) > 0 ) { // wait for CPU to finish
				if(messageBuffer!=NULL) puts(messageBuffer); // if it's "Elestac" we're happy.
				break;
			}
		}else{
			puts("Somebody is calling");
			write(clientSocket , "Identify your self" , strlen(messageBuffer));
			break;
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
int CallUMC(){
	int clientUMC;
	//char* aPCB = PCB; // won't use it now -> Send "kernel"
	char aPCB[] = "KERNEL";
	struct sockaddr_in server;
	char *clientMessage = NULL, *serverMessage = NULL;
    //Create socket
    if(getClientSocket(&clientUMC, UMC_IP, UMC_PORT)) {
    	return (-1);
    }
    clientMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
    if(clientMessage == NULL) {
      puts("===Error in messageBuffer malloc===");
      return (-1);
    }
    serverMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
    if(serverMessage == NULL) {
      puts("===Error in messageBuffer malloc===");
      return (-1);
    }
    //keep communicating with server
  //  while(1) {
      //  printf("Enter message : ");
     //   fgets(clientMessage, PACKAGE_SIZE, stdin);
     //   clientMessage[strlen(clientMessage) - 1] = '\0';

        //Send some data
        if( send(clientUMC , aPCB , PACKAGE_SIZE , 0) < 0) {
            puts("Send failed");
            return 1;
        }
        //Receive a reply from the server
        if( recv(clientUMC , serverMessage , PACKAGE_SIZE , 0) < 0) {
            puts("recv failed");
        //    break;
        }
        puts("UMC reply :");
        puts(serverMessage);
        clientMessage[0] = '\0';
        serverMessage[0] = '\0';
 //   }
    close(clientUMC);
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
