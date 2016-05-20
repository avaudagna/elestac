/*
 UMC -> SE COMUNICA CON SWAP , RECIBE CONEXIONES CON CPU Y NUCLEO
 */
/*INCLUDES*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>

/*MACROS y TIPOS DE DATOS*/
#define PUERTO "6750"
#define BACKLOG 1			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
#define RETARDO 1
#define DUMP    2
#define FLUSH   3
#define SALIR   (-1)

#define PUERTO_SWAP 1234
#define IP_SWAP "127.0.0.1"


typedef struct umc_parametros {
     int	core_cpu_port,
	       	ip_swap,
			port_swap,
			marcos,
			marcos_size,
			retardo;
}UMC_PARAMETERS;

typedef void (*FunctionPointer)(int *);
typedef void * (*boid_function_boid_pointer) (void*);

/*Global Variables */
int socketServidor;
int socketClienteSwap;
UMC_PARAMETERS Umc_Global_Parameters;
int contConexionesNucleo = 0;
//int contConexionesCPU = 0;

void *  connection_handler(void * socketCliente);
void Init_UMC(void);
void Init_Socket(void);
void Init_Swap(void);
void HandShake_Swap(void);
//void Init_Parameters(void);
void *  funcion_menu(void * noseusa);
void Imprimir_Menu(void);
void Menu_UMC(void);
void Procesar_Conexiones(void);
FunctionPointer QuienSos( int * _socketCliente);
void TestKernel(int *);
void TestCPU(int *);

int main(){

	Init_UMC();
	Menu_UMC();

  while(1)
	 	 {
        	Procesar_Conexiones();
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
  Init_Socket(); // socket de escucha
  Init_Swap(); // socket con swap
  HandShake_Swap();

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

	

}

void * funcion_menu (void * noseusa)
{
     int opcion = 0,flag=0;
     while(!flag)
     {
          Imprimir_Menu();
          printf("\nIngresar opcion deseada");
          scanf("%d",&opcion);
          switch(opcion)
          {
               case RETARDO:
                    printf("\nRETARDO!!!!!");
                    break;
               case DUMP:
                    printf("\nDUMP!!!!!");
                    break;
               case FLUSH:
                    printf("\nFLUSH!!!!!");
                    break;
               case SALIR:
                    flag = 1;
                    break;
               default:
                    break;
          }
     }
printf("\n FINALIZA EL THREAD DEL MENU \n!!!!");
pthread_exit(0);
}

void Imprimir_Menu(void)
{
     printf("\nBienvenido al menu de UMC\n");
}


void Procesar_Conexiones (void)
{
	/*DECLARACION DE SOCKETS*/
	int socketBuffer, *socketCliente = NULL;
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
    pthread_t *thread_id;				// pthread pointer, por cada nueva conexion , creo uno nuevo
    FunctionPointer ConnectionHandler;

      if( ( socketBuffer = accept(socketServidor, (struct sockaddr *) &addr, &addrlen) ) == -1 ) {
    	  perror("accept");
    	  exit(1);
		}
	    printf("\nConnected..\n");

	    socketCliente = (int *) malloc ( sizeof(int));
	    *socketCliente = socketBuffer;
	    thread_id = (pthread_t * ) malloc (sizeof(pthread_t));		// new thread

	ConnectionHandler = QuienSos(socketCliente);

	if( pthread_create( thread_id , NULL , (boid_function_boid_pointer) ConnectionHandler , (void*) &socketCliente) < 0)
		  {
			perror("pthread_create");
			exit(1);
		  }
		//pthread_join(thread_id,NULL);
	
}



void Menu_UMC(void)
{
  pthread_t thread_menu;
      if( pthread_create( &thread_menu , NULL ,  funcion_menu ,(void *) NULL) )
        {
                perror("pthread_create");
                exit(1);
        }
}


FunctionPointer QuienSos( int * _socketCliente) {


	FunctionPointer aux = NULL;
	int   socketCliente					= *( (int *) _socketCliente),
		  cantidad_de_bytes_recibidos 	= 0;
	char *package 						= NULL;

	package = (char *) malloc(sizeof(char) * PACKAGESIZE ) ;
	cantidad_de_bytes_recibidos = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
	if ( cantidad_de_bytes_recibidos <= 0 ) {
		if ( cantidad_de_bytes_recibidos < 0 )
			perror("recv");
		else
			return NULL;
	}

		//package[strlen(package) - 1] = '\0';
		printf("\nCliente: ");
		printf("%s\n", package);

	if ( (strcmp(package,"KERNEL") == 0 ) ) {	// KERNEL
		 contConexionesNucleo++;


		if ( contConexionesNucleo == 1 )
			{
				if ( send(socketCliente,"UMC",PACKAGESIZE,0) == -1 ) {
						perror("send");
						exit(1);
					  }

				 aux = TestKernel;
				 return aux;

			}
		else{
				if ( send(socketCliente,(void *)"Error:Cantidad de conexiones KERNEL!!",strlen("Error:Cantidad de conexiones NUCLEO")+1,0) == -1 ) {
						perror("send");
						exit(1);
					  }
		 }
	}

	if (( strcmp(package,"CPU") ) == 0 ){   //CPU

		 if ( send(socketCliente,(void *)"a=b+3",PACKAGESIZE,0) == -1 ) {
	 	 	 perror("send");
	 	 	 exit(1);
 	 	  }

			 aux = TestCPU;
			 return aux;
	}

	free(package);
	return aux;

}


void TestKernel(int * socketBuff ){

	printf("\nHola , soy el thread encargado de la comunicacion con el Kernel!! :)");
	contConexionesNucleo--; // finaliza la comunicacion con el socket
	close(*socketBuff); // cierro socket
	pthread_exit(0);	// chau thread

}
void TestCPU(int * socketBuff){

	printf("\nHola , soy el thread encargado de la comunicacion con el CPU!! :)");

	close(*socketBuff);		// cierro socket
	pthread_exit(0);		// chau thread

}


void Init_Swap(void){

		struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

		getaddrinfo(IP_SWAP, PUERTO_SWAP, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

		socketClienteSwap = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);


		if ( connect(socketClienteSwap, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1 ){
			perror("connect");
			exit(1);
		}
		freeaddrinfo(serverInfo);	// No lo necesitamos mas

}

void HandShake_Swap(void){
 // enviar trama : U+tamPag
 // esperar 1(correcto)+cantidad_de_paginas_libres
}


