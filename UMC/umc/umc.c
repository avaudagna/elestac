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
bool filtrarPorNroPagina ( t_list *headerTablaPaginas);


#define SIZE_HANDSHAKE_KERNEL 5


// TODO LO QUE ES CON SWAP
#define PUERTO_SWAP "6500"
#define IP_SWAP "127.0.0.1"
#define SIZE_HANDSHAKE_SWAP 5 // 'U' 1 Y 4 BYTES PARA LA CANTIDAD DE PAGINAS
//#define PAGE_SIZE 1024

// Funciones para operar con el swap
void Init_Swap(void);
void HandShake_Swap(void);
void SwapUpdate(void);

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
void Init_UMC(char *);
void Init_Socket(void);
void Init_Parameters(char * configFile);
void loadConfig(char* configFile);
void Init_MemoriaPrincipal(void);
//void Init_Parameters(void);
void *  funcion_menu(void * noseusa);
void Imprimir_Menu(void);
void Menu_UMC(void);
void Procesar_Conexiones(void);
FunctionPointer QuienSos(int * socketBuff);
void AtenderCPU(int * socketBuff);
int setServerSocket(int* serverSocket, const char* address, const int port);
void CambioProcesoActivo(int * socket,int * pid);
void PedidoBytes(int * socketBuff,int *pid_actual);

int main(int argc , char **argv){

	if(argc != 2){
		printf("Error cantidad de argumentos, finalizando...");
		exit(1);
	}

	Init_UMC(argv[1]);
	Menu_UMC();

  while(1)
	 	 {
        	Procesar_Conexiones();
         }	

}

void Init_UMC(char * configFile)
{
    Init_Parameters(configFile);
    Init_MemoriaPrincipal();
	Init_Swap(); // socket con swap
	HandShake_Swap();
	Init_Socket(); // socket de escucha
}


void Init_Parameters (char * configFile){

	loadConfig(configFile);
	headerListaDePids = list_create();
}


void Init_MemoriaPrincipal(void){
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

void Init_Socket(void){
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

		if ( contConexionesNucleo == 1 ) {

			aux = AtenderKernel;
			return aux;

			}

	}

   if ( package == '1' ){   //CPU

	   contConexionesCPU++;
	   aux = AtenderCPU;
	   return aux;

	}

	return aux;

}

void AtenderCPU(int * socketBuff){

	int pid_actual,estado=CAMBIO_PROCESO_ACTIVO;

	while(estado != EXIT ){
		switch(estado){

		case IDENTIFICADOR_OPERACION:
			estado = IdentificarOperacion(socketBuff);
			break;
		case CAMBIO_PROCESO_ACTIVO:
			CambioProcesoActivo(socketBuff,&pid_actual);
			estado = IDENTIFICADOR_OPERACION;
			break;
		case PEDIDO_BYTES:
			PedidoBytes(socketBuff,&pid_actual);
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

void CambioProcesoActivo(int * socket,int * pid){

	if ( (recv(*socket, (void*) (pid), 4, 0)) <= 0 )	// levanto byte que indica que tipo de operacion se va a llevar a cabo
		perror("recv");

}

void AtenderKernel(int * socketBuff){

	int estado = HANDSHAKE;	// la primera vez que entra,es porque ya recibi un 0 indicando handshake

	while(estado != EXIT)
	{
		switch(estado)
		{
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


int IdentificarOperacion(int * socketBuff){

	int package;

	while ( (recv(*socketBuff, (void*) (&package), 1, 0)) < 0 );	// levanto byte que indica que tipo de operacion se va a llevar a cabo

	return package;

}

void  HandShakeKernel(int * socketBuff){

	int cantidad_bytes_recibidos;
	char buffer_stack_size[4];
	cantidad_bytes_recibidos = recv(*socketBuff, (void*) buffer_stack_size, 4, 0);		// levanto los 4 bytes q son del stack_size
	stack_size = atoi(buffer_stack_size);
	printf("\nStack Size : %d\n",stack_size);
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

	if ( (cantidadDePaginasSolicitadas+stack_size) <= paginasLibresEnSwap) {	// Se puede almacenar el nuevo proceso , respondo que SI

		RecibirYAlmacenarNuevoProceso(socketBuff,cantidadDePaginasSolicitadas,pid_aux);

		char trama_handshake[4];
		sprintf(trama_handshake,"%04d",cantidadDePaginasSolicitadas);

		if ( send(*socketBuff,(void *)trama_handshake,4,0) == -1)
			perror("send");

	}else{

		//char trama_handshake[2]={'N','O'};

		if ( send(*socketBuff,(void *)"0000",4,0) == -1 )
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

		if( (recv(*socketBuff, (void*) (codigo), code_size, 0)) <= 0){	// me almaceno todo el codigo
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
				// tengo : pid+nroDePagina+aux_code
			sprintf(trama,"%04d",pid_aux);
			sprintf(&trama[sizeof(pid_aux)],"%04d",nroDePagina);
			memcpy((void * )&trama[sizeof(pid_aux)+sizeof(nroDePagina)],aux_code,umcGlobalParameters.marcosSize);

			// envio al swap la pagina
			EnviarTramaAlSwap(trama,umcGlobalParameters.marcosSize+sizeof(pid_aux)+sizeof(nroDePagina));
			// swap me responde dandome el OKEY y actualizandome con la cantidad_de_paginas_libres que le queda
			SwapUpdate();
			page_node = (PAGINA *)malloc (sizeof(PAGINA));
			page_node->nroPagina=nroDePagina;
			page_node->modificado=0;
			page_node->presencia=0;
			// agrego pagina a la tabla de paginas asociada a ese PID
			AgregarPagina(headerListaDePids,pid_aux,page_node);

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

void SwapUpdate(void){
// 1+CANTIDAD_DE_PAGINAS_LIBRES ( 5 bytes )

	char package[5];

	if ( recv(socketClienteSwap, (void*) package, SIZE_HANDSHAKE_SWAP, 0) > 0 ){
		if ( package[0] == '1')
			paginasLibresEnSwap = atoi(package+1);
	}else
		perror("recv");



}

void Init_Swap(void){

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

void HandShake_Swap(void){
 // enviar trama : U+tamPag
 // esperar de SWAP 1(correcto)+cantidad_de_paginas_libres

	char *package = NULL;	// recepcion
	char *buffer= NULL;
	char trama_handshake[SIZE_HANDSHAKE_SWAP];
	buffer = (char * )malloc (SIZE_HANDSHAKE_SWAP);

	sprintf(buffer,"U%d",umcGlobalParameters.marcosSize);

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
void PedidoBytes(int * socketBuff,int *pid_actual){

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

	headerTablaDePaginas = ObtenerTablaDePaginasDePID(headerListaDePids,*pid_actual);





}