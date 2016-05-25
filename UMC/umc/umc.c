/*
 UMC -> SE COMUNICA CON SWAP , RECIBE CONEXIONES CON CPU Y NUCLEO
 */
/*INCLUDES*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <errno.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>

#define IDENTIFICADOR_MODULO 1

/*MACROS y TIPOS DE DATOS*/
#define PUERTO "6750"
#define BACKLOG 1			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar
#define RETARDO 1
#define DUMP    2
#define FLUSH   3
#define SALIR   (-1)

/* COMUNICACION/OPERACIONES CON KERNEL*/
#define REPOSO 3
#define IDENTIFICADOR_OPERACION 99
#define HANDSHAKE 0
#define SOLICITUD_NUEVO_PROCESO 1
#define FINALIZAR_PROCESO 2
#define EXIT (-1)
#define PID_SIZE 4

typedef struct umc_parametros {
     int	core_cpu_port,
	       	ip_swap,
			port_swap,
			marcos,
			marcos_size,
			retardo;
}UMC_PARAMETERS;


typedef struct _pagina {
	int nroPagina;
	int presencia;
	int modificado;
}PAGINA;

typedef struct _pidPaginas {
	int pid;
	t_list * headListaDePaginas;
}PIDPAGINAS;

t_list *headerListaDePids;


// Funciones para/con KERNEL
int IdentificarOperacion(int * socketBuff);
void HandShakeKernel(int * socketBuff);
void ProcesoSolicitudNuevoProceso(int * socketBuff);
void FinalizarProceso(int * socketBuff);
void AtenderKernel(int * socketBuff);
void RecibirYAlmacenarNuevoProceso(int * socketBuff, int cantidadPaginasSolicitadas, int pid_aux);
void DividirEnPaginas( int cantidadPaginasSolicitadas,int pid_aux, int paginasDeCodigo,char codigo[],int code_size);
void EnviarTramaAlSwap(char trama[],int size_trama);
void AgregarPagina(t_list * header,int pid,PAGINA * pagina_aux);
t_list * ObtenerTablaDePaginasDePID(t_list *header,int pid);
bool CompararPid(PAGINA * data,int pid);
t_list * GetHeadListaPaginas(PIDPAGINAS * data);

bool filtrarPorPid ( void * );
bool filtrarPorNroPagina ( void *);


#define SIZE_HANDSHAKE_KERNEL 5


// TODO LO QUE ES CON SWAP
#define PUERTO_SWAP "6500"
#define IP_SWAP "127.0.0.1"
#define SIZE_HANDSHAKE_SWAP 5 // 'U' 1 Y 4 BYTES PARA LA CANTIDAD DE PAGINAS
#define PAGE_SIZE 1024

// Funciones para operar con el swap
void Init_Swap(void);
void HandShake_Swap(void);

typedef void (*FunctionPointer)(int * socketBuff);
typedef void * (*boid_function_boid_pointer) (void*);

/*VARIABLES GLOBALES */
int socketServidor;
int socketClienteSwap;
UMC_PARAMETERS Umc_Global_Parameters;
int contConexionesNucleo = 0;
//int contConexionesCPU = 0;
int paginasLibresEnSwap = 0;
int stack_size;

void *  connection_handler(void * socketCliente);
void Init_UMC(void);
void Init_Socket(void);
//void Init_Parameters(void);
void *  funcion_menu(void * noseusa);
void Imprimir_Menu(void);
void Menu_UMC(void);
void Procesar_Conexiones(void);
//FunctionPointer QuienSos( int * _socketCliente, PARAMETROS_HILO **param);

FunctionPointer QuienSos(int * socketBuff);
void AtenderCPU(int * socketBuff);

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
	Init_Swap(); // socket con swap
	HandShake_Swap();
	Init_Socket(); // socket de escucha
}

/*
 * void Init_Parameters (char * file_conf){
 *
 * 		//leo el file y cargo la estructura Umc_Global_Parameters
 * 		// headerListaDePids = list_create();
 *
 * }
 *
 *
 *
 * */

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
    //PARAMETROS_HILO * param = NULL;

      if( ( socketBuffer = accept(socketServidor, (struct sockaddr *) &addr, &addrlen) ) == -1 ) {
    	  perror("accept");
    	  exit(1);
		}
	    printf("\nConnected..\n");

	    socketCliente = (int *) malloc ( sizeof(int));
	    *socketCliente = socketBuffer;
	    thread_id = (pthread_t * ) malloc (sizeof(pthread_t));	// new thread
	    //param = (PARAMETROS_HILO *) malloc ( sizeof(PARAMETROS_HILO));

	    //param->socketBuff = socketCliente;
	    ConnectionHandler = QuienSos(socketCliente);
	//ConnectionHandler = QuienSos(socketCliente,&param);

	if( pthread_create( thread_id , NULL , (boid_function_boid_pointer) ConnectionHandler , (void*) socketCliente) < 0)
	  //if( pthread_create( thread_id , NULL , (boid_function_boid_pointer) ConnectionHandler , (void*) param ) < 0)
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


//FunctionPointer QuienSos( int * _socketCliente , PARAMETROS_HILO **param) {
FunctionPointer QuienSos(int * socketBuff) {

	FunctionPointer aux = NULL;
	int   socketCliente					= *socketBuff,
		  cantidad_de_bytes_recibidos 	= 0;
	//char *package 						= NULL;
	char package;

	//package = (char *) malloc(sizeof(char) * IDENTIFICADOR_MODULO ) ;
	cantidad_de_bytes_recibidos = recv(socketCliente, (void*) (&package), IDENTIFICADOR_MODULO, 0);
	if ( cantidad_de_bytes_recibidos <= 0 ) {
		if ( cantidad_de_bytes_recibidos < 0 )
			perror("recv");
		else
			return NULL;
	}

		//package[strlen(package) - 1] = '\0';
		//printf("\nCliente: ");
		//printf("%s\n", package);

	//if ( (strcmp(package,"K") == 0 ) ) {	// KERNEL

	if ( package == 'K' ) {	// KERNEL
		contConexionesNucleo++;

		if ( contConexionesNucleo == 1 ) {

			//(*param)->package = (char * ) malloc (cantidad_de_bytes_recibidos + 1);
			//strcpy((*param)->package,package++);	// me saco de encima la K
			aux = AtenderKernel;
			return aux;

			}
		else{
				if ( send(socketCliente,(void *)"Error:Cantidad de conexiones KERNEL!!",strlen("Error:Cantidad de conexiones NUCLEO")+1,0) == -1 ) {
						perror("send");
						exit(1);
					  }
		 }
	}

	// if (( strcmp(package,"C") ) == 0 ){   //CPU

		if ( package == 'C' ) {	// CPU
			// FALTA GESTIONAR CONEXION CPU
		 if ( send(socketCliente,(void *)"a=b+3",PACKAGESIZE,0) == -1 ) {
	 	 	 perror("send");
	 	 	 exit(1);
 	 	  }

			 aux = AtenderCPU;
			 return aux;
	}

	//free(package);
	return aux;

}

void AtenderCPU(int * socketBuff){

	printf("\nHola , soy el thread encargado de la comunicacion con el CPU!! :)");

	close(*socketBuff);		// cierro socket
	pthread_exit(0);		// chau thread

}


void AtenderKernel(int * socketBuff){

	printf("\nHola , soy el thread encargado de la comunicacion con el Kernel!! :)");
	int estado = IDENTIFICADOR_OPERACION;	// la primera vez que entra,es porque ya recibi una 'K'

	while(estado != EXIT)
	{
		switch(estado)
		{
		/*case REPOSO:
			estado = EsperandoKernel();		// A la espera de algun tipo de operacion
			break;
		*/
		case IDENTIFICADOR_OPERACION:
			estado = IdentificarOperacion(socketBuff);
			break;
		case HANDSHAKE:	// K0
				HandShakeKernel(socketBuff);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case SOLICITUD_NUEVO_PROCESO:		// 1
				ProcesoSolicitudNuevoProceso(socketBuff);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case FINALIZAR_PROCESO:	// 2
			FinalizarProceso(socketBuff);
			estado = REPOSO;
			break;
		default:
			printf("Identificador de operacion invalido");
			estado=EXIT;
			break;

		}
	}

	contConexionesNucleo--; // finaliza la comunicacion con el socket

	close(*socketBuff); // cierro socket
	pthread_exit(0);	// chau thread

}


int IdentificarOperacion(int * socketBuff){

	int package;

	while ( (recv(*socketBuff, (void*) (&package), 1, 0)) <= 0 );	// levanto byte que indica que tipo de operacion se va a llevar a cabo

	return package;

}

void  HandShakeKernel(int * socketBuff){

	// recibo STACK_SIZE
	//char *buffer_stack = NULL;
	//buffer_stack = (char *) malloc(4);
	//memcpy(buffer_stack,package+2,4);
	int cantidad_bytes_recibidos;

	cantidad_bytes_recibidos = recv(*socketBuff, (void*) (&stack_size), sizeof(stack_size), 0);		// levanto los 4 bytes q son del stack_size

	if ( cantidad_bytes_recibidos <= 0){
		perror("recv");
	}

	// devuelvo U0+TAMAÑO PAGINA

	char *buffer= NULL;
	char trama_handshake[SIZE_HANDSHAKE_KERNEL];
		buffer = (char * )malloc (SIZE_HANDSHAKE_KERNEL);

		sprintf(buffer,"U%d",PAGE_SIZE);

		int i = 0;

		// le quito el \0 al final

		for(i=0;i<SIZE_HANDSHAKE_SWAP;i++){
			trama_handshake[i]=buffer[i];
		}

		if ( send(*socketBuff,(void *)trama_handshake,SIZE_HANDSHAKE_SWAP,0) == -1 ) {
				perror("send");
				exit(1);
			}

}

/* TRAMA : 1+PID+CANT PAGs+CODE SIZE+CODE
 **/

void ProcesoSolicitudNuevoProceso(int * socketBuff){

	int cantidadDePaginasSolicitadas,
		cantidad_bytes_recibidos,
		pid_aux;

	// levanto los 4 bytes q son del pid
	cantidad_bytes_recibidos = recv(*socketBuff, (void*) (&pid_aux), sizeof(pid_aux), 0);

		if ( cantidad_bytes_recibidos <= 0){
			perror("recv");
		}

	// levanto los 4 bytes q son del cantidadDePaginasSolicitadas
	cantidad_bytes_recibidos = recv(*socketBuff, (void*) (&cantidadDePaginasSolicitadas), sizeof(cantidadDePaginasSolicitadas), 0);

		if ( cantidad_bytes_recibidos <= 0){
				perror("recv");
		}

	if ( cantidadDePaginasSolicitadas < paginasLibresEnSwap) {	// Se puede almacenar el nuevo proceso , respondo que SI

		RecibirYAlmacenarNuevoProceso(socketBuff,cantidadDePaginasSolicitadas,pid_aux);

		char trama_handshake[2]={'S','I'};

		if ( send(*socketBuff,(void *)trama_handshake,2,0) == -1)
			perror("send");

	}else{

		char trama_handshake[2]={'N','O'};

		if ( send(*socketBuff,(void *)trama_handshake,2,0) == -1 )
			perror("send");
	}

}

/*TRAMA : 1+PID+CANT PAGs+CODE SIZE+CODE
 * */
void RecibirYAlmacenarNuevoProceso(int * socketBuff,int cantidadPaginasSolicitadas, int pid_aux){

// obtengo code_size

	int code_size;

	PIDPAGINAS nuevo_pid;

	nuevo_pid.pid = pid_aux;
	nuevo_pid.headListaDePaginas = NULL;

	list_add(headerListaDePids,(void *)&(nuevo_pid));		//agrego nuevo PID

	if( (recv(*socketBuff, (void*) (&code_size), sizeof(code_size), 0)) <= 0){
		perror("recv");
	}else{
		// obtengo codigo
		int paginasDeCodigo = code_size / 5;  //definir parametro tamanio pagina !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		char codigo[code_size];

		if( (recv(*socketBuff, (void*) (codigo), code_size, 0)) <= 0){
				perror("recv");
		}
		else{
			  DividirEnPaginas(cantidadPaginasSolicitadas,pid_aux,paginasDeCodigo,codigo,code_size);
		}
	}

}

/*1+pid+numPagina+codigo*/
void DividirEnPaginas( int cantidadPaginasSolicitadas,int pid_aux, int paginasDeCodigo,char codigo[],int code_size){

	int i = 0,
		nroDePagina=0,
		j = 0,
		bytes_restantes=0;
	char * aux_code,
		   trama[page_size+sizeof(pid_aux)+sizeof(nroDePagina)];  // PAGE SIZE !!!!!!!!!!!!!
	PAGINA * page_node;

/*Cosas a Realizar por cada pagina :
 * 1. Armo la trama para enviarle a SWAP
 * 2. Envio la trama al SWAP
 * 2. Agrego a la tabla de paginas asociada a ese PID
 * */

	for ( i=0; i < paginasDeCodigo ; i++ , nroDePagina++ , j+=page_size ){

		aux_code = (char * ) malloc( page_size );
		if ( nroDePagina < (paginasDeCodigo - 1) ){
			memcpy((void * )aux_code,&codigo[j],page_size);

		}else{	// ultima pagina
			bytes_restantes = ( paginasDeCodigo * page_size ) - code_size;
				if (!bytes_restantes)	// si la cantidad de bytes es multiplo del tamaño de pagina , entonces la ultima pagina se llena x completo
					memcpy((void * )aux_code,&codigo[j],page_size);
				else
					memcpy((void * )aux_code,&codigo[j],bytes_restantes);

				// tengo : pid+nroDePagina+aux_code
			sprintf(trama,"%04d",pid_aux);
			sprintf(&trama[sizeof(pid_aux)],"%04d",nroDePagina);
			memcpy((void * )&trama[sizeof(pid_aux)+sizeof(nroDePagina)],aux_code,page_size);

			// envio al swap la pagina
			EnviarTramaAlSwap(trama,page_size+sizeof(pid_aux)+sizeof(nroDePagina));

			page_node = (PAGINA *)malloc (sizeof(PAGINA));
			page_node->nroPagina=nroDePagina;
			page_node->modificado=0;
			page_node->presencia=0;
			// agrego pagina a la tabla de paginas asociada a ese PID
			AgregarPagina(headerListaDePids,pid_aux,page_node);


		}


	}

}

void EnviarTramaAlSwap(char trama[],int size_trama){

	if ( send(socketClienteSwap,(void *)trama,size_trama,0) == -1 ){
		perror("send");
		exit(1);
	}

}

void AgregarPagina(t_list * headerListaDePids,int pid, PAGINA *pagina_aux){

	t_list * aux = NULL;

	aux = ObtenerTablaDePaginasDePID(headerListaDePids,pid);

	list_add(aux,pagina_aux);
}

t_list * ObtenerTablaDePaginasDePID(t_list *header,int pid){

	t_link_element *aux_pointer = header->head;

   while(aux_pointer != NULL){

		 if (CompararPid(aux_pointer->data,pid))		// retornar puntero a lista de paginas
			return (GetHeadListaPaginas(aux_pointer->data));

	    	aux_pointer = aux_pointer->next;

	}

   return NULL;

}


bool CompararPid(PAGINA * data,int pid){
	if (data->nroPagina == pid)
		return true;
	else
		return false;
}

t_list * GetHeadListaPaginas(PIDPAGINAS * data){

	if (data->headListaDePaginas == NULL)
		data->headListaDePaginas = list_create();

	return data->headListaDePaginas;
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
 // esperar de SWAP 1(correcto)+cantidad_de_paginas_libres

	char *package = NULL;	// recepcion
	char *buffer= NULL;
	char trama_handshake[SIZE_HANDSHAKE_SWAP];
	buffer = (char * )malloc (SIZE_HANDSHAKE_SWAP);

	sprintf(buffer,"U%d",PAGE_SIZE);

	int i = 0;

	// le quito el \0 al final

	for(i=0;i<SIZE_HANDSHAKE_SWAP;i++){
		trama_handshake[i]=buffer[i];
	}

	if ( send(socketClienteSwap,(void *)trama_handshake,SIZE_HANDSHAKE_SWAP,0) == -1 ) {
			perror("send");
			exit(1);
		}
	// SWAP me responde 1+CANTIDAD_DE_PAGINAS_LIBRES ( 1 byte + 4 de cantidad de paginas libres )
	package = (char *) malloc(sizeof(char) * SIZE_HANDSHAKE_SWAP) ;
	if ( recv(socketClienteSwap, (void*) package, SIZE_HANDSHAKE_SWAP, 0) > 0 ){

		if ( package[0] == '1'){
			//  paginasLibresEnSwap = los 4 bytes que quedan
			char *aux=NULL;
			aux = (char *) malloc(4);
			memcpy(aux,package+1,4);
			paginasLibresEnSwap = atoi(aux);
			printf("\nSe ejecuto correctamente el handshake");
		}

	}
	else{
		perror("recv");
		exit(1);
	}

}


