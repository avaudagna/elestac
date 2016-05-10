#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include "socketCommons/socketCommons.h"
#include <signal.h>

#define KERNEL_ADDR "127.0.0.1"
#define KERNEL_PORT 54326

#define PACKAGE_SIZE 10

void tratarSeniales(int);
int main(int argc , char *argv[])
{
	signal (SIGINT, tratarSeniales);
	int kernelSocketClient;
	char kernel_reply[2000];
	//create kernel client
	if(argc != 3) {
		puts("usage: console conf file");
		return -1; }
	getClientSocket(&kernelSocketClient, KERNEL_ADDR, KERNEL_PORT);

	/*le voy a mandar al kernel una T0 (terminal),
	y me va a devolver el client_id (un número) que me represente con el cual él me conoce
	Luego T1+ sizeMsj en 4B +(todo el código como viene)*/
	//receive a reply from the kernel
	send(kernelSocketClient, "T0", PACKAGE_SIZE, 0);

	do {
		if( recv(kernelSocketClient , kernel_reply , PACKAGE_SIZE , 0) < 0) {
			puts("recv failed");
			break;
		}
		 puts("Kernel reply :");
		 puts(kernel_reply);

	}
	/*
	 * OJO con este while, ya no recibo "kernel"...
	 */
	while(strcmp(kernel_reply, "kernel"));

	// le envío a kernel T1+sizeMsj en 4B+mensaje

	FILE * fp;
	fp = fopen (argv[2], "r");
	fseek(fp, SEEK_SET, 0);

	fseek(fp, 0L, SEEK_END);
	long int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	//Despues haces malloc con sz
	char* prog = (char*) malloc(sz);

	fread(prog, sz+1, 1, fp);
	fclose(fp);

	int sizeMsj = strlen("T1")+5+(int) sz;
	char* mensaje = (char*) malloc(sizeMsj);

    char buffer [20];
    sprintf(buffer, "%04d\n", (int) sz);

	strcpy(mensaje, "T1");
	strcat(mensaje, buffer);
	strcat(mensaje, prog);
	send(kernelSocketClient, mensaje, sizeMsj, 0);
	//printf("Sent: %s/n", fp);
	printf("Sent: %s/n", mensaje);
	//send(kernelSocketClient, "#VamoACalmarno",PACKAGE_SIZE, 0);
	//puts("Sent: #VamoACalmarno");
	//do {
	//	if( recv(kernelSocketClient , kernel_reply , PACKAGE_SIZE , 0) < 0) {
//			puts("recv failed");
//			break;
	//	}
	//} while(strcmp(kernel_reply, "nop"));
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
			printf("Esto acabará con el sistema. Presione Ctrl+C una vez más para confirmar.\n\n");
			signal (SIGINT, SIG_DFL); // solo controlo una vez.
			break;
	}
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

