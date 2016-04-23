#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include "vac-commons/socketCommons.h"

#define KERNEL_ADDR "10.0.0.12"
#define KERNEL_PORT 6669

int main(int argc , char *argv[])
{
	int kernelSocketClient;
	char kernel_reply[2000];
	//create kernel client
	getClientSocket(&kernelSocketClient, KERNEL_ADDR, KERNEL_PORT);

	//receive a reply from the kernel
	send(kernelSocketClient, "console", 8, 0);

	do {
		if( recv(kernelSocketClient , kernel_reply , 2000 , 0) < 0) {
			puts("recv failed");
			break;
		}
		 puts("Kernel reply :");
		 puts(kernel_reply);
	} while(strcmp(kernel_reply, "kernel"));

	send(kernelSocketClient, "hola", 5, 0);
	send(kernelSocketClient, "#VamoACalmarno");
	do {
		if( recv(kernelSocketClient , kernel_reply , 2000 , 0) < 0) {
			puts("recv failed");
			break;
		}
	} while(strcmp(kernel_reply, "nop"));
	close(kernelSocketClient);
	    return 0;

}





    /*
    1) ./ejemplo.ansisop(porAhoraHola)
    2) consola ("consola") -> kernel ("kernell") -> consola (porAhoraHola) -> kernel
    3) kernel -> cpu
    4)kernel (porAhoraHola) -> umc (porAhoraHola NO) -> kernel (porAhoraHola NO) -> consola
    5) CPU (porAhoraHola) -> UMC
    6) UMC (porAhoraHola) -> swap
     */

    /*lógica de loopeo
    send(sock, argv[1], sizeof(argv[1]), 0);
    //así como está, por ahora manda el programa entero

    //keep communicating with server
    while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 )
        {

    	char* dummy = NULL;
        dummy = (char*) malloc(sizeof(mail));
        strcpy(dummy,client_message);
        dummy = strtok(dummy, "-");

    	if(!strcmp(dummy, "IM")){
    		//tengo que hacer dos veces strtok porque tengo que imprimir dos parámetros
    		*user = (char*) malloc(sizeof(dummy));
    		strcpy(*user, dummy);
    		param_1 = strtok(NULL, "@");
    		*parametro = (char*) malloc(sizeof(param_1));
    		strcpy(*parametro, param_1);

    		*user = (char*) malloc(sizeof(dummy));
    		strcpy(*user, dummy);
    		param_2 = strtok(NULL, "@");
    		*parametro = (char*) malloc(sizeof(param_2));
    		strcpy(*parametro, param_2);

    		printf("%s : %s", param_1, param_2);

    	}

    	else if (!strcmp(dummy, "IT")){
    		*user = (char*) malloc(sizeof(dummy));
    		strcpy(*user, dummy);
    		param_1 = strtok(NULL, "@");
    		*parametro = (char*) malloc(sizeof(param_1));
    		strcpy(*parametro, param_1);



    		printf("%s : %s", param_1);

    	}



    		//free(dummy);
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

    {


        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }
        //el núcleo debería mandarme el print y print_lalala (ya lo estoy recibiendo en el if anterior)
        // meter if acá?
        puts("Server reply :");
        puts(server_reply);
    }
    */

