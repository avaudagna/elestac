/*
 * Modelo ejemplo de un servidor que espera mensajes de un proceso Cliente que se conecta a un cierto puerto.
 * Al recibir un mensaje, lo imprimira por pantalla.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#define PUERTO "6669"
#define BACKLOG 1			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar


typedef struct umc_parametros {
     int core_cpu_port,ip_swap,port_swap,marcos,marcos_size,retardo;
}UMC_PARAMETERS;



/*Global Variables */
int socketServidor;
UMC_PARAMETERS Umc_Global_Parameters;

void *  connection_handler(void * socketCliente);
void Init_UMC(void);
void Init_Socket(void);
//void Init_Parameters(void);
void funcion_menu(void);


int main(){

	/*DECLARACION DE SOCKETS*/
	int socketCliente;
	pthread_t thread_menu;
	
	if( pthread_create( &thread_menu , NULL ,  funcion_menu , NULL)
	   {
			perror("pthread_create");
			exit(1);
	   }

	
	Init_UMC();

	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
        pthread_t thread_id;				// pthread que va a ir atendiendo las distintas llegadas de conexiones
	
  while(1)
	 {
	      
	      if( ( socketCliente = accept(socketServidor, (struct sockaddr *) &addr, &addrlen) ) == -1 ) {
		perror("accept");
		exit(1);
		}
	    printf("\nConnected..\n");

		if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &socketCliente) < 0)
		  {
			perror("pthread_create");
			exit(1);
		  }
		//pthread_join(thread_id,NULL);
	   }	

	//close(socketCliente);
	close(socketServidor);

	/* See ya! */

	return 0;
}

void *  connection_handler(void * _socket)
{
 int   socketCliente			= *( (int *) _socket),
       cantidad_de_bytes_recibidos 	= 0,
       done				= 0;
 char *package 				= NULL;
 
 		
	//  close(socketServidor);  // NO LO CIERRO PORQUE LOS FILE DESCRIPTOR SE COMPARTEN ENTRE LOS THREADS
		       printf("Cliente conectado. Esperando mensajes:\n");
	    	do
	     	  {

			package = (char *) malloc(sizeof(char) * PACKAGESIZE ) ;
			cantidad_de_bytes_recibidos = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
			if ( cantidad_de_bytes_recibidos <= 0 ) {
				if ( cantidad_de_bytes_recibidos < 0 ) perror("recv");
				done = 1;
			  }	
			if (!done) {
				package[cantidad_de_bytes_recibidos]='\0';
		   		printf("\nCliente: ");
				printf("%s", package);
				if(!strcmp(package,"Adios"))break;
				fflush(stdin);
				printf("\nServidor: ");
				fgets(package,PACKAGESIZE,stdin);
				package[strlen(package) - 1] = '\0';
		    		if ( send(socketCliente,package,strlen(package)+1,0) == -1 ) { 			
			 	perror("send");
			 	exit(1);
		       		}
			 free(package);
			}	
		
	     	  }while(!done);
	close(socketCliente);
	pthread_exit(0);
}

void Init_UMC(void)
{
//  Init_Parameters();
  Init_Socket();

}

void Init_Socket(void)
{
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, PUERTO, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE


	/*
	 * 	Descubiertos los misterios de la vida (por lo menos, para la conexion de red actual), necesito enterarme de alguna forma
	 * 	cuales son las conexiones que quieren establecer conmigo.
	 *
	 * 	Para ello, y basandome en el postulado de que en Linux TODO es un archivo, voy a utilizar... Si, un archivo!
	 *
	 * 	Mediante socket(), obtengo el File Descriptor que me proporciona el sistema (un integer identificador).
	 *
	 */
	/* Necesitamos un socket que escuche las conecciones entrantes */
	
	if ( ( socketServidor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol) ) == -1 ) {
	   perror("socket");
	   exit(1);
	  }

	/*
	 * 	Perfecto, ya tengo un archivo que puedo utilizar para analizar las conexiones entrantes. Pero... ¿Por donde?
	 *
	 * 	Necesito decirle al sistema que voy a utilizar el archivo que me proporciono para escuchar las conexiones por un puerto especifico.
	 *
	 * 				OJO! Todavia no estoy escuchando las conexiones entrantes!
	 *
	 */
	if ( bind(socketServidor,serverInfo->ai_addr, serverInfo->ai_addrlen) == -1 ) {
	   perror("bind");
	   exit(1);
	  }
	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar

	/*
	 * 	Ya tengo un medio de comunicacion (el socket) y le dije por que "telefono" tiene que esperar las llamadas.
	 *
	 * 	Solo me queda decirle que vaya y escuche!
	 *
	 */
	if ( listen(socketServidor, BACKLOG) == -1 ) {		// IMPORTANTE: listen() es una syscall BLOQUEANTE.
	   perror("listen");
 	   exit(1);
	  }

	/*
	 * 	El sistema esperara hasta que reciba una conexion entrante...
	 * 	...
	 * 	...
	 * 	BING!!! Nos estan llamando! ¿Y ahora?
	 *
	 *	Aceptamos la conexion entrante, y creamos un nuevo socket mediante el cual nos podamos comunicar (que no es mas que un archivo).
	 *
	 *	¿Por que crear un nuevo socket? Porque el anterior lo necesitamos para escuchar las conexiones entrantes. De la misma forma que
	 *	uno no puede estar hablando por telefono a la vez que esta esperando que lo llamen, un socket no se puede encargar de escuchar
	 *	las conexiones entrantes y ademas comunicarse con un cliente.
	 *
	 *			Nota: Para que el socketServidor vuelva a esperar conexiones, necesitariamos volver a decirle que escuche, con listen();
	 *				En este ejemplo nos dedicamos unicamente a trabajar con el cliente y no escuchamos mas conexiones.
	 *
	 */
	
}

