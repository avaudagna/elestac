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
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <errno.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/collections/node.h>

#define IDENTIFICADOR_MODULO 1

/*MACROS */
#define BACKLOG 10			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define RETARDO 123
#define DUMP    2
#define FLUSH   3
#define SALIR   (-1)
#define LIBRE 	0
#define OCUPADO 1

/* COMUNICACION/OPERACIONES CON KERNEL*/
#define REPOSO 3
#define IDENTIFICADOR_OPERACION 99
#define HANDSHAKE 0
#define SOLICITUD_NUEVO_PROCESO 1
#define FINALIZAR_PROCESO 2
#define EXIT (-1)
#define PID_SIZE 4
#define SIZE_OF_PAGE_SIZE 4

/* COMUNICACION/OPERACIONES CON CPU */

#define PEDIDO_BYTES 2
#define ALMACENAMIENTO_BYTES 3
#define FIN_COMUNICACION_CPU 0
#define CAMBIO_PROCESO_ACTIVO 1



typedef struct umc_parametros {
     int	listenningPort,
			marcos,
			marcosSize,
			marcosXProc,
			retardo,
			entradasTLB;
     char   *algoritmo,
		    *ipSwap,
			*listenningIp,
			*portSwap;
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

typedef struct _marco {
	void *comienzoMarco;
	unsigned char estado;
}MARCO;




// Funciones para/con KERNEL
int  identificarOperacion(int * socketBuff);
void handShakeKernel(int * socketBuff);
void procesoSolicitudNuevoProceso(int * socketBuff);
void FinalizarProceso(int * socketBuff);
void atenderKernel(int * socketBuff);
void recibirYAlmacenarNuevoProceso(int * socketBuff, int cantidadPaginasSolicitadas, int pid_aux);
void dividirEnPaginas(int pid_aux, int paginasDeCodigo, char codigo[], int code_size);
void enviarPaginaAlSwap(char *trama, int size_trama);
void agregarPagina(t_list * header,int pid,PAGINA * pagina_aux);
t_list * obtenerTablaDePaginasDePID(t_list *header,int pid);
bool compararPid(PAGINA * data,int pid);
t_list * getHeadListaPaginas(PIDPAGINAS * data);

bool filtrarPorPid ( void * );
bool filtrarPorNroPagina ( t_list *headerTablaPaginas);


#define SIZE_HANDSHAKE_KERNEL 5


// TODO LO QUE ES CON SWAP
#define PUERTO_SWAP "6500"
#define IP_SWAP "127.0.0.1"
#define SIZE_handShake_Swap 5 // 'U' 1 Y 4 BYTES PARA LA CANTIDAD DE PAGINAS
//#define PAGE_SIZE 1024

// Funciones para operar con el swap
void init_Swap(void);
void handShake_Swap(void);
void swapUpdate(void);

typedef void (*FunctionPointer)(int * socketBuff);
typedef void * (*boid_function_boid_pointer) (void*);

/*VARIABLES GLOBALES */
int socketServidor;
int socketClienteSwap;
UMC_PARAMETERS umcGlobalParameters;
int contConexionesNucleo = 0;
int contConexionesCPU = 0;
int paginasLibresEnSwap = 0;
int stack_size;
t_list *headerListaDePids;
void *memoriaPrincipal = NULL;
MARCO *vectorMarcos = NULL;

/*FUNCIONES DE INICIALIZACION*/
void init_UMC(char *);
void init_Socket(void);
void init_Parameters(char *configFile);
void loadConfig(char* configFile);
void init_MemoriaPrincipal(void);
void *  funcion_menu(void * noseusa);
void imprimir_Menu(void);
void Menu_UMC(void);
void procesarConexiones(void);
FunctionPointer QuienSos(int * socketBuff);
void atenderCPU(int *socketBuff);
int setServerSocket(int* serverSocket, const char* address, const int port);
void cambioProcesoActivo(int *socket, int *pid);
void pedidoBytes(int *socketBuff, int *pid_actual);
void enviarPaginasDeStackAlSwap(int _pid, int nroDePaginaInicial);
void indexarPaginasDeStack(int _pid, int nroDePagina);

int main(int argc , char **argv){

	if(argc != 2){
		printf("Error cantidad de argumentos, finalizando...");
		exit(1);
	}

	init_UMC(argv[1]);
	Menu_UMC();

  while(1)
	 	 {
			 procesarConexiones();
         }	

}

void init_UMC(char * configFile)
{
	init_Parameters(configFile);
	init_MemoriaPrincipal();
	init_Swap(); // socket con swap
	handShake_Swap();
	init_Socket(); // socket de escucha
}


void init_Parameters(char *configFile){

	loadConfig(configFile);
	headerListaDePids = list_create();
}


void init_MemoriaPrincipal(void){
	int i=0;
	memoriaPrincipal = malloc(umcGlobalParameters.marcosSize * umcGlobalParameters.marcos);	// inicializo memoria principal
	vectorMarcos = (MARCO *) malloc (sizeof(MARCO) * umcGlobalParameters.marcos) ;

	for(i=0;i<umcGlobalParameters.marcos;i++){
		vectorMarcos[i].estado=LIBRE;
		vectorMarcos[i].comienzoMarco = memoriaPrincipal+(i*umcGlobalParameters.marcosSize);
	}
}


void loadConfig(char* configFile){

	if(configFile == NULL){
		printf("\nArchivo de configuracion invalido. Finalizando UMC\n");
		exit(1);
	}
	else{

		t_config *config = config_create(configFile);
		puts(" .:: Loading settings ::.");

		umcGlobalParameters.listenningPort=config_get_int_value(config,"PUERTO");
		umcGlobalParameters.listenningIp=config_get_string_value(config,"IP_ESCUCHA");
		umcGlobalParameters.ipSwap=config_get_string_value(config,"IP_SWAP");
		umcGlobalParameters.portSwap=config_get_string_value(config,"PUERTO_SWAP");
		umcGlobalParameters.marcos=config_get_int_value(config,"MARCOS");
		umcGlobalParameters.marcosSize=config_get_int_value(config,"MARCO_SIZE");
		umcGlobalParameters.marcosXProc=config_get_int_value(config,"MARCO_X_PROC");
		umcGlobalParameters.algoritmo=config_get_string_value(config,"ALGORITMO");
		umcGlobalParameters.entradasTLB=config_get_int_value(config,"ENTRADAS_TLB");
		umcGlobalParameters.retardo=config_get_int_value(config,"RETARDO");

		//config_destroy(config);

	}
}

void init_Socket(void){
	setServerSocket(&socketServidor,umcGlobalParameters.listenningIp,umcGlobalParameters.listenningPort);
}

int setServerSocket(int* serverSocket, const char* address, const int port) {
	struct sockaddr_in serverConf;
	*serverSocket = socket(AF_INET, SOCK_STREAM , 0);
	if (*serverSocket == -1) puts("Could not create socket.");
	serverConf.sin_family = AF_INET;
	serverConf.sin_addr.s_addr = inet_addr(address);
	serverConf.sin_port = htons(port);
	if( bind(*serverSocket,(struct sockaddr *)&serverConf, sizeof(serverConf)) < 0) {
		perror("Bind failed.");
		return (-1);
	}
	listen(*serverSocket , BACKLOG); // 3 is my queue of clients
	return 0;
}

void * funcion_menu (void * noseusa)
{
     int opcion = 0,flag=0;
     while(!flag)
     {
		 imprimir_Menu();
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

void imprimir_Menu(void)
{
     printf("\nBienvenido al menu de UMC\n");
}


void procesarConexiones(void)
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
	    thread_id = (pthread_t * ) malloc (sizeof(pthread_t));	// new thread

	    ConnectionHandler = QuienSos(socketCliente);

	if( pthread_create( thread_id , NULL , (boid_function_boid_pointer) ConnectionHandler , (void*) socketCliente) < 0) {
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



FunctionPointer QuienSos(int * socketBuff) {

	FunctionPointer aux = NULL;
	int   socketCliente					= *socketBuff,
		  cantidad_de_bytes_recibidos 	= 0;
	char package;

	cantidad_de_bytes_recibidos = recv(socketCliente, (void*) (&package), IDENTIFICADOR_MODULO, 0);
	if ( cantidad_de_bytes_recibidos <= 0 ) {
		if ( cantidad_de_bytes_recibidos < 0 )
			perror("recv");
		else
			return NULL;
	}

	if ( package == '0' ) {	// KERNEL
		contConexionesNucleo++;

		printf("\n.: Se abre conexion con KERNEL :. \n");

		if ( contConexionesNucleo == 1 ) {

			aux = atenderKernel;
			return aux;

			}

	}

   if ( package == '1' ){   //CPU

	   contConexionesCPU++;
	   aux = atenderCPU;
	   return aux;

	}

	return aux;

}

void atenderCPU(int *socketBuff){

	int pid_actual,estado=CAMBIO_PROCESO_ACTIVO;

	while(estado != EXIT ){
		switch(estado){

		case IDENTIFICADOR_OPERACION:
			estado = identificarOperacion(socketBuff);
			break;
		case CAMBIO_PROCESO_ACTIVO:
			cambioProcesoActivo(socketBuff, &pid_actual);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case PEDIDO_BYTES:
			pedidoBytes(socketBuff, &pid_actual);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case ALMACENAMIENTO_BYTES:
			//AlmacenarBytes(socketBuff,&pid_actual);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case FIN_COMUNICACION_CPU:
			estado = EXIT;
			break;
		}

	}
	contConexionesCPU--;
	close(*socketBuff);		// cierro socket
	pthread_exit(0);		// chau thread
}

void cambioProcesoActivo(int *socket, int *pid){

	if ( (recv(*socket, (void*) (pid), 4, 0)) <= 0 )	// levanto byte que indica que tipo de operacion se va a llevar a cabo
		perror("recv");

}

void atenderKernel(int * socketBuff){

	int estado = HANDSHAKE;	// la primera vez que entra,es porque ya recibi un 0 indicando handshake

	while(estado != EXIT)
	{
		switch(estado)
		{
		case IDENTIFICADOR_OPERACION:
			estado = identificarOperacion(socketBuff);
			break;
		case HANDSHAKE:	// K0
				handShakeKernel(socketBuff);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case SOLICITUD_NUEVO_PROCESO:		// 1
				procesoSolicitudNuevoProceso(socketBuff);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case FINALIZAR_PROCESO:	// 2
			//FinalizarProceso(socketBuff);
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


int identificarOperacion(int * socketBuff){

	int package;
	char *buffer = malloc(2);


	while ( (recv(*socketBuff, (void*) (buffer), 1, 0)) < 0 );	// levanto byte que indica que tipo de operacion se va a llevar a cabo

	package=atoi(buffer);

	return package;

}

void  handShakeKernel(int * socketBuff){

	int cantidad_bytes_recibidos;
	char buffer_stack_size[4];
	cantidad_bytes_recibidos = recv(*socketBuff, (void*) buffer_stack_size, 4, 0);		// levanto los 4 bytes q son del stack_size
	stack_size = atoi(buffer_stack_size);
	printf("\n.:Stack Size : %d :.\n",stack_size);
	if ( cantidad_bytes_recibidos <= 0){
		perror("recv");
	}

	// devuelvo TAMAÑO PAGINA

	char buffer[SIZE_OF_PAGE_SIZE];

	sprintf(buffer,"%04d",umcGlobalParameters.marcosSize);

	if ( send(*socketBuff,(void *)buffer,SIZE_OF_PAGE_SIZE,0) == -1 ) {
			perror("send");
			exit(1);
		}

}

/* TRAMA : 1+PID+CANT PAGs+CODE SIZE+CODE
 **/

void procesoSolicitudNuevoProceso(int * socketBuff){

	int cantidadDePaginasSolicitadas,
		cantidad_bytes_recibidos,
		pid_aux;
	char buffer[4];

	// levanto los 4 bytes q son del pid
	cantidad_bytes_recibidos = recv(*socketBuff, (void*) (buffer), 4, 0);

	if ( cantidad_bytes_recibidos <= 0)	perror("recv");

	pid_aux = atoi(buffer);

	// levanto los 4 bytes q son del cantidadDePaginasSolicitadas
	cantidad_bytes_recibidos = recv(*socketBuff, (void*) (buffer), 4, 0);

	if ( cantidad_bytes_recibidos <= 0)	perror("recv");

	cantidadDePaginasSolicitadas = atoi(buffer);


	if ( (cantidadDePaginasSolicitadas+stack_size) <= paginasLibresEnSwap) {	// Se puede almacenar el nuevo proceso , respondo que SI

		recibirYAlmacenarNuevoProceso(socketBuff,cantidadDePaginasSolicitadas,pid_aux);

		char trama_handshake[4];
		sprintf(trama_handshake,"%04d",cantidadDePaginasSolicitadas);

		if ( send(*socketBuff,(void *)trama_handshake,4,0) == -1) perror("send");

	}else
		if ( send(*socketBuff,(void *)"0000",4,0) == -1 ) perror("send");
}

/*TRAMA : 1+PID+CANT PAGs+CODE SIZE+CODE
 * */
void recibirYAlmacenarNuevoProceso(int * socketBuff,int cantidadPaginasSolicitadas, int pid_aux){

// obtengo code_size

	int code_size;
	PIDPAGINAS nuevo_pid;
	char buffer[4];

	nuevo_pid.pid = pid_aux;
	nuevo_pid.headListaDePaginas = NULL;
	list_add(headerListaDePids,(void *)&(nuevo_pid));		//agrego nuevo PID

	// levanto el campo tamaño de codigo
	if( (recv(*socketBuff, (void*) (buffer), 4 , 0)) <= 0){
		perror("recv");
	}else{
		// levanto el codigo
		int paginasDeCodigo = cantidadPaginasSolicitadas;  // cantidadPaginasSolicitadas + stack_size
		char codigo[code_size];

		if( (recv(*socketBuff, (void*) (codigo), code_size, 0)) <= 0){	// me almaceno todo el codigo
				perror("recv");
		}
		else{
			dividirEnPaginas(pid_aux, paginasDeCodigo, codigo, code_size);
		}
	}

}

/*1+pid+numPagina+codigo*/
void dividirEnPaginas(int pid_aux, int paginasDeCodigo, char codigo[], int code_size) {

	int i = 0,
		nroDePagina=0,
		j = 0,
		bytes_restantes=0;
	char * aux_code,
		   trama[umcGlobalParameters.marcosSize+sizeof(pid_aux)+sizeof(nroDePagina)];
	PAGINA * page_node;

/*Cosas a Realizar por cada pagina :
 * 1. Armo la trama para enviarle a SWAP
 * 2. Envio la trama al SWAP
 * 2. Agrego a la tabla de paginas asociada a ese PID
 * */

	for ( i=0,j=0; i < paginasDeCodigo ; i++ , nroDePagina++ , j+=umcGlobalParameters.marcosSize ){

		aux_code = (char * ) malloc( umcGlobalParameters.marcosSize );
		if ( nroDePagina < (paginasDeCodigo - 1) ){
			memcpy((void * )aux_code,&codigo[j],umcGlobalParameters.marcosSize);

		}else {    // ultima pagina
			bytes_restantes = (paginasDeCodigo * umcGlobalParameters.marcosSize) - code_size;
			if (!bytes_restantes)    // si la cantidad de bytes es multiplo del tamaño de pagina , entonces la ultima pagina se llena x completo
				memcpy((void *) aux_code, &codigo[j], umcGlobalParameters.marcosSize);
			else
				memcpy((void *) aux_code, &codigo[j],
					   bytes_restantes);    // LO QUE NO SE LLENA CON INFO, SE DEJA EN GARBAGE
		}
				// trama de escritura a swap : 1+pid+nroDePagina+aux_code
			sprintf(trama,"1%04d",pid_aux);
			sprintf(&trama[sizeof(pid_aux)],"%04d",nroDePagina);
			memcpy((void * )&trama[sizeof(pid_aux)+sizeof(nroDePagina)],aux_code,umcGlobalParameters.marcosSize);

			// envio al swap la pagina
			enviarPaginaAlSwap(trama, umcGlobalParameters.marcosSize + sizeof(pid_aux) + sizeof(nroDePagina));
			// swap me responde dandome el OKEY y actualizandome con la cantidad_de_paginas_libres que le queda
			swapUpdate();
			page_node = (PAGINA *)malloc (sizeof(PAGINA));
			page_node->nroPagina=nroDePagina;
			page_node->modificado=0;
			page_node->presencia=0;
			// agrego pagina a la tabla de paginas asociada a ese PID
			agregarPagina(headerListaDePids,pid_aux,page_node);

	}
	// una vez que finaliza de enviar paginas de codigo , ahora envio las stack_size paginas de STACK :D
	enviarPaginasDeStackAlSwap(pid_aux,nroDePagina);
}

void indexarPaginasDeStack(int _pid, int nroDePagina) {

	PAGINA * page_node;

	page_node = (PAGINA *)malloc (sizeof(PAGINA));
	page_node->nroPagina=nroDePagina;
	page_node->modificado=0;
	page_node->presencia=0;
	// agrego pagina a la tabla de paginas asociada a ese PID
	agregarPagina(headerListaDePids,_pid,page_node);
}

void enviarPaginasDeStackAlSwap(int _pid, int nroDePaginaInicial) {

	int i=0;
	char trama[sizeof(_pid)+sizeof(nroDePaginaInicial)+umcGlobalParameters.marcosSize];

	// trama de escritura a swap : 1+pid+nroDePagina+aux_code
	sprintf(trama,"1%04d",_pid);

	for(i=0;i<stack_size;i++,nroDePaginaInicial++){

		sprintf(&trama[sizeof(pid_aux)],"%04d",nroDePaginaInicial);
		enviarPaginaAlSwap(trama,umcGlobalParameters.marcosSize + ( sizeof(_pid) * 2 ) ) ;
		indexarPaginasDeStack(_pid,nroDePaginaInicial);
	}

}

void enviarPaginaAlSwap(char *trama, int size_trama){

	if ( send(socketClienteSwap,(void *)trama,size_trama,0) == -1 ){
		perror("send");
		exit(1);
	}

}

void agregarPagina(t_list * headerListaDePids,int pid, PAGINA *pagina_aux){

	t_list * aux = NULL;

	aux = obtenerTablaDePaginasDePID(headerListaDePids,pid);

	list_add(aux,pagina_aux);
}

t_list * obtenerTablaDePaginasDePID(t_list *header,int pid){

	t_link_element *aux_pointer = header->head;

   while(aux_pointer != NULL){

		 if (compararPid(aux_pointer->data,pid))		// retornar puntero a lista de paginas
			return (getHeadListaPaginas(aux_pointer->data));

	    	aux_pointer = aux_pointer->next;

	}

   return NULL;

}


bool compararPid(PAGINA * data,int pid){
	if (data->nroPagina == pid)
		return true;
	else
		return false;
}

t_list * getHeadListaPaginas(PIDPAGINAS * data){

	if (data->headListaDePaginas == NULL)
		data->headListaDePaginas = list_create();

	return data->headListaDePaginas;
}

void swapUpdate(void){
// 1+CANTIDAD_DE_PAGINAS_LIBRES ( 5 bytes )

	char package[5];

	if ( recv(socketClienteSwap, (void*) package, SIZE_handShake_Swap, 0) > 0 ){
		if ( package[0] == '1')
			paginasLibresEnSwap = atoi(package+1);
	}else
		perror("recv");



}

void init_Swap(void){

		struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

		getaddrinfo(umcGlobalParameters.ipSwap, umcGlobalParameters.portSwap, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

		socketClienteSwap = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);


		if ( connect(socketClienteSwap, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1 ){
			perror("connect");
			exit(1);
		}
		freeaddrinfo(serverInfo);	// No lo necesitamos mas

}

void handShake_Swap(void){
 // enviar trama : U+tamPag
 // esperar de SWAP 1(correcto)+cantidad_de_paginas_libres

	char *package = NULL;	// recepcion
	char *buffer= NULL;
	char trama_handshake[SIZE_handShake_Swap];
	buffer = (char * )malloc (SIZE_handShake_Swap);

	sprintf(buffer,"U%d",umcGlobalParameters.marcosSize);

	int i = 0;

	// le quito el \0 al final

	for(i=0;i<SIZE_handShake_Swap;i++){
		trama_handshake[i]=buffer[i];
	}

	if ( send(socketClienteSwap,(void *)trama_handshake,SIZE_handShake_Swap,0) == -1 ) {
			perror("send");
			exit(1);
		}
	// SWAP me responde 1+CANTIDAD_DE_PAGINAS_LIBRES ( 1 byte + 4 de cantidad de paginas libres )
	package = (char *) malloc(sizeof(char) * SIZE_handShake_Swap) ;
	if ( recv(socketClienteSwap, (void*) package, SIZE_handShake_Swap, 0) > 0 ){

		if ( package[0] == 'S'){
			//  paginasLibresEnSwap = los 4 bytes que quedan
			char *aux=NULL;
			aux = (char *) malloc(4);
			memcpy(aux,package+1,4);
			paginasLibresEnSwap = atoi(aux);
			//printf("\nSe ejecuto correctamente el handshake");
		}

	}
	else{
		perror("recv");
		exit(1);
	}

}

// 2+PAGINA+OFFSET+TAMAÑO
/* 1- Levanto toda la trama
 * 2- Voy a la tabla de paginas correspondiente
 * 3- Me fijo si la tabla esta o no
 * 4-
 */
void pedidoBytes(int *socketBuff, int *pid_actual){

	int _pagina,_offset,_tamanio;
	char buffer[4];
	t_list *headerTablaDePaginas = NULL;

	// levanto nro de pagina
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_pagina=atoi(buffer);

	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_offset=atoi(buffer);

	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_tamanio=atoi(buffer);

	headerTablaDePaginas = obtenerTablaDePaginasDePID(headerListaDePids,*pid_actual);





}