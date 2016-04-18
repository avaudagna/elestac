/*
 * Modelo ejemplo de un Cliente que envia mensajes a un Server.
 *
 * 	No se contemplan el manejo de errores en el sistema por una cuestion didactica. Tener en cuenta esto al desarrollar.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define IP "127.0.0.1"
#define PUERTO "6669"
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

int main(){

	/*
	 *  ¿Quien soy? ¿Donde estoy? ¿Existo?
	 *
	 *  Estas y otras preguntas existenciales son resueltas getaddrinfo();
	 *
	 *  Obtiene los datos de la direccion de red y lo guarda en serverInfo.
	 *
	 */
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(IP, PUERTO, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion


	/*
	 * 	Ya se quien y a donde me tengo que conectar... ¿Y ahora?
	 *	Tengo que encontrar una forma por la que conectarme al server... Ya se! Un socket!
	 *
	 * 	Obtiene un socket (un file descriptor -todo en linux es un archivo-), utilizando la estructura serverInfo que generamos antes.
	 *
	 */
	int serverSocket;
	serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	/*
	 * 	Perfecto, ya tengo el medio para conectarme (el archivo), y ya se lo pedi al sistema.
	 * 	Ahora me conecto!
	 *
	 */
	 if ( (connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1 ) {
	   perror("connect");
	   exit(1);
	 }
	freeaddrinfo(serverInfo);	// No lo necesitamos mas

	/*
	 *	Estoy conectado! Ya solo me queda una cosa:
	 *
	 *	Enviar datos!
	 *
	 *	Vamos a crear un paquete (en este caso solo un conjunto de caracteres) de size PACKAGESIZE, que le enviare al servidor.
	 *
	 *	Aprovechando el standard immput/output, guardamos en el paquete las cosas que ingrese el usuario en la consola.
	 *	Ademas, contamos con la verificacion de que el usuario escriba "exit" para dejar de transmitir.
	 *
	 */
	int enviar = 1,cantidad_de_bytes_recibidos=0;
	char *message;
	

	printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

	while(1){
		//while(getchar()!='\0');
		message=(char*)malloc(sizeof(char)*PACKAGESIZE);
		printf("\nCliente: ");
		fgets(message,PACKAGESIZE,stdin);
		message[ strlen(message) - 1 ] = '\0';	
		
		//fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (!strcmp(message,"exit")) {
			free(message);
			send(serverSocket,"Adios",6,0);
			break;
		  }
		if (enviar) { 
                     if ( ( send(serverSocket, message, strlen(message) + 1, 0) ) == -1 )
			{ 	
			  perror("send");
			  exit(1);
			}
		}
                if ( ( cantidad_de_bytes_recibidos=recv(serverSocket,message,PACKAGESIZE,0) ) == -1 )
		   {
			perror("recv");
			exit(1);
	           }
		if (cantidad_de_bytes_recibidos) // si es el que el servidor me respondio algo
		  {
			message[cantidad_de_bytes_recibidos]='\0';
			printf("\nServidor:%s\n",message);
		  }
		free(message);
		}

	/*
	 *	Listo! Cree un medio de comunicacion con el servidor, me conecte con y le envie cosas...
	 *
	 *	...Pero me aburri. Era de esperarse, ¿No?
	 *
	 *	Asique ahora solo me queda cerrar la conexion con un close();
	 */

	close(serverSocket);
	return 0;

	/* ADIO'! */
}
