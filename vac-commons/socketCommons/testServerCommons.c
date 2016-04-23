
#include "socketCommons.h"
#define LOCALHOST "127.0.0.1"

int main (int argc, char **argv) {

  int serverSocket, port = 8080;
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
    //Send the message back to client
    write(clientSocket , messageBuffer , strlen(messageBuffer)); //echo
  }

  if(read_size == 0) {
    puts("Client disconnected");
    fflush(stdout);
  }

  else if(read_size == -1) {
    perror("recv failed");
  }

  return 0;
}
