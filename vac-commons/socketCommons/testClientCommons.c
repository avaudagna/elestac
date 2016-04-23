
#include "socketCommons.h"
#define LOCALHOST "127.0.0.1"

int main(int argc , char *argv[]) {
    int clientSocket, port = 8080;
    struct sockaddr_in server;
    char *clientMessage = NULL, *serverMessage = NULL;

    //Create socket
    if(getClientSocket(&clientSocket, LOCALHOST, port)) {
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
    while(1) {
        printf("Enter message : ");
        fgets(clientMessage, PACKAGE_SIZE, stdin);
        clientMessage[strlen(clientMessage) - 1] = '\0';

        //Send some data
        if( send(clientSocket , clientMessage , strlen(clientMessage) , 0) < 0) {
            puts("Send failed");
            return 1;
        }

        //Receive a reply from the server
        if( recv(clientSocket , serverMessage , PACKAGE_SIZE , 0) < 0) {
            puts("recv failed");
            break;
        }
        puts("Server reply :");
        puts(serverMessage);
        clientMessage[0] = '\0';
        serverMessage[0] = '\0';
    }

    close(clientSocket);
    return 0;
}
