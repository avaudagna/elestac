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
#define SIZE_OF_PAGE_SIZE 4

/* COMUNICACION/OPERACIONES CON CPU */

#define HANDSHAKE_CPU 1
#define CAMBIO_PROCESO_ACTIVO 2
#define PEDIDO_BYTES 3
#define ALMACENAMIENTO_BYTES 4
#define FIN_COMUNICACION_CPU 0

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
	int nroDeMarco;
}PAGINA;

typedef struct _pidPaginas {
	int pid;
	int cantPagEnMP;
	t_list * headListaDePaginas;
}PIDPAGINAS;

typedef struct clock {
	int pid;
	t_list * headerFifo;
}CLOCK_PID;

typedef struct clock_pagina {
	int nroPagina,
		bitDeUso,
		bitDeModificado;
}CLOCK_PAGINA;

typedef struct fifo_indice {
	int pid,
		indice;
}FIFO_INDICE;

typedef struct _marco {
	void *comienzoMarco;
	unsigned char estado;
}MARCO;

typedef struct _tlb {
	int pid,
		nroPagina,
		indice,
		estado;	// numero de marco en memoria principal
}TLB;


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
bool compararPid(PIDPAGINAS * data,int pid);

t_list * getHeadListaPaginas(PIDPAGINAS * data);


PAGINA * obtenerPaginaDeTablaDePaginas(t_list *headerTablaPaginas, int nroPagina);


#define SIZE_HANDSHAKE_SWAP 5 // 'U' 1 Y 4 BYTES PARA LA CANTIDAD DE PAGINAS
#define PRESENTE_MP 1
#define AUSENTE 0

#define ESCRITURA 1
#define LECTURA	3

// Funciones para operar con el swap
void init_Swap(void);
void handShake_Swap(void);
void swapUpdate(void);

typedef void (*FunctionPointer)(int * socketBuff);
typedef void * (*boid_function_boid_pointer) (void*);

/*VARIABLES GLOBALES */
UMC_PARAMETERS umcGlobalParameters;
int 	contConexionesNucleo = 0,
	 	contConexionesCPU = 0,
		paginasLibresEnSwap = 0,
		socketServidor,
		socketClienteSwap,
		stack_size;
t_list 	*headerListaDePids,
		*headerTLB = NULL,
		*headerPunterosClock = NULL,		// aca almaceno puntero a pagina x cada PID para saber cual fue la ultima referenciada
		*headerFIFOxPID = NULL;
void 	*memoriaPrincipal = NULL;
MARCO 	*vectorMarcos = NULL;



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

bool comparaNroDePagina(PAGINA *pagina, int nroPagina);

void pedidoDePaginaInvalida(int *socketBuff);

void resolverEnMP(int *socketBuff, PAGINA *pPagina, int offset, int tamanio);

void *pedirPaginaSwap(int *socketBuff, int *pid_actual, int nroPagina, int *tamanioContenidoPagina);

void almacenoPaginaEnMP(int *pPid, int pPagina, char codigo[], int tamanioPagina);

void *obtenerMarcoLibreEnMP(int *indice);

int marcosDisponiblesEnMP();

int cantPagDisponiblesxPID(int *pPid);

void setBitDePresencia(int pPid, int pPagina, int estado);

void setNroDeMarco(int *pPid, int pPagina, int indice);

PAGINA *obtenerPagina(int pPid, int pPagina);

int getcantPagEnMP(PIDPAGINAS *pPidPagina);

t_link_element *obtenerPidPagina(int pPid);

int getcantPagEnMPxPID(int pPid);

void setcantPagEnMPxPID(int pPid, int nuevocantPagEnMP);

void init_TLB(void);

bool consultarTLB(int *pPid, int pagina, int *indice);

t_link_element *obtenerIndiceTLB(int *pPid, int pagina);

void setBitDeUso(int pPid, int pPagina, int valorBitDeUso);

void algoritmoClock(int pPid, int numPagNueva, int tamanioContenidoPagina, void *contenidoPagina);

bool filtrarPorBitDePresencia(void *data);

void handShakeCpu(int *socketBuff);


t_list *obtenerHeaderFifoxPid(t_list *headerFifos, int pPid);
CLOCK_PID * obtenerCurrentClockPidEntry(t_list * headerFifos, int pid);

bool pidEstaEnListaFIFOxPID(t_list *headerFifos, int pPid);

bool pidEstaEnListaDeUltRemp(t_list *headerPunteros, int pPid);

CLOCK_PAGINA * obtenerPaginaDeFifo(t_list *fifoDePid, int pagina);

FIFO_INDICE * obtenerPunteroClockxPID(t_list *clock, int pid);

t_link_element * list_get_nodo(t_list *self, int index);

void PedidoPaginaASwap(int pid, int pagina, int operacion);

void AlmacenarBytes(int *socketBuff, int *pid_actual);

void guardarBytesEnPagina(int *pid_actual, int pagina, int offset, int tamanio, void *bytesAGuardar);

void setBitModificado(int pPid, int pagina, int valorBitModificado);

void EnviarOKACPU(int socketBuff, int pPid);

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
	init_TLB();
	init_Swap(); // socket con swap
	handShake_Swap();
	init_Socket(); // socket de escucha
}

void init_TLB(void) {

	if ( umcGlobalParameters.entradasTLB > 0){
		headerTLB = list_create();
		int i = 0;
		TLB * aux = NULL;
		for ( i=0 ; i < umcGlobalParameters.entradasTLB ; i++){		// le pongo a la lista TLB tantos nodos como entradas tenga
			aux = (TLB *) malloc(sizeof(TLB));
			aux->pid=0;
			aux->nroPagina=0;
			aux->indice=0;
			aux->estado=LIBRE;
			list_add(headerTLB,aux);
		}
	}

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

	int pid_actual,estado=HANDSHAKE_CPU;

	while(estado != EXIT ){
		switch(estado){

		case IDENTIFICADOR_OPERACION:
			estado = identificarOperacion(socketBuff);
			break;
		case HANDSHAKE_CPU:
				handShakeCpu(socketBuff);
				estado = IDENTIFICADOR_OPERACION;
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
			AlmacenarBytes(socketBuff,&pid_actual);
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



void handShakeCpu(int *socketBuff) {

	char buffer[5];

	// 0+PAGE_SIZE
	sprintf(buffer,"0%04d",umcGlobalParameters.marcosSize);

	if ( send(*socketBuff,(void *)buffer,5,0) == -1 )
		perror("send");

}

void cambioProcesoActivo(int *socket, int *pid){

	char buffer[4];
	// levanto el nuevo PID que esta ejecutando el CPU
	if ( (recv(*socket, (void*) (buffer), 4, 0)) <= 0 )
		perror("recv");
	*pid = atoi(buffer);

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

	char buffer[sizeof(int)];

	sprintf(buffer,"%04d", umcGlobalParameters.marcosSize);

	if ( send(*socketBuff,(void *)buffer, sizeof(int),0) == -1 ) {
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

	nuevo_pid.cantPagEnMP=0;
	nuevo_pid.pid = pid_aux;
	nuevo_pid.headListaDePaginas = NULL;
	list_add(headerListaDePids,(void *)&(nuevo_pid));		//agrego nuevo PID

	// levanto el campo tamaño de codigo
	if( (recv(*socketBuff, (void*) (buffer), 4 , 0)) <= 0){
		perror("recv");
	}else{
		// levanto el codigo
		int paginasDeCodigo = cantidadPaginasSolicitadas;  // cantidadPaginasSolicitadas + stack_size
		code_size = atoi(buffer);
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
		bytesSobrantes=0,
        bytesDeUltimaPagina=0,
		size_trama = umcGlobalParameters.marcosSize+sizeof(pid_aux)+sizeof(nroDePagina) + 1;
	char * aux_code,
	     * trama = NULL;
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
			bytesSobrantes = (paginasDeCodigo * umcGlobalParameters.marcosSize) - code_size;
			bytesDeUltimaPagina = umcGlobalParameters.marcosSize - bytesSobrantes;
			if (!bytesSobrantes)    // si la cantidad de bytes es multiplo del tamaño de pagina , entonces la ultima pagina se llena x completo
				memcpy((void *) aux_code, &codigo[j], umcGlobalParameters.marcosSize);
			else
				memcpy((void *) aux_code, &codigo[j], bytesDeUltimaPagina);    // LO QUE NO SE LLENA CON INFO, SE DEJA EN GARBAGE
			}
			// trama de escritura a swap : 1+pid+nroDePagina+aux_code
			asprintf(&trama, "%d%04d%04d", 1, pid_aux, nroDePagina);
			memcpy((void * )&trama[size_trama - umcGlobalParameters.marcosSize],aux_code,umcGlobalParameters.marcosSize);

			// envio al swap la pagina
			enviarPaginaAlSwap(trama, size_trama);
			// swap me responde dandome el OKEY y actualizandome con la cantidad_de_paginas_libres que le queda
			swapUpdate();
			page_node = (PAGINA *)malloc (sizeof(PAGINA));
			page_node->nroPagina=nroDePagina;
			page_node->modificado=0;
			page_node->presencia=0;
			page_node->nroDeMarco=-1;
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
	page_node->nroDeMarco=-1;
	// agrego pagina a la tabla de paginas asociada a ese PID
	agregarPagina(headerListaDePids,_pid,page_node);
}

void enviarPaginasDeStackAlSwap(int _pid, int nroDePaginaInicial) {

	int nroPaginaActual=0,
        i = 0;
	char * trama = NULL;

	// trama de escritura a swap : 1+pid+nroDePagina+aux_code
	for(i =0 ,nroPaginaActual=nroDePaginaInicial; i<stack_size; i++, nroPaginaActual++) {
        asprintf(&trama, "%d%04d%04d", 1, _pid, nroPaginaActual);
        trama = realloc(trama, strlen(trama) + umcGlobalParameters.marcosSize);
//		enviarPaginaAlSwap(trama,umcGlobalParameters.marcosSize + ( sizeof(_pid) * 2 ) ) ;
        //mando una pagina con basura
		enviarPaginaAlSwap(trama, sizeof(char) + sizeof(int) + sizeof(int) + umcGlobalParameters.marcosSize);
		swapUpdate();
		indexarPaginasDeStack(_pid,nroPaginaActual);
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


bool compararPid(PIDPAGINAS * data,int pid){
	if (data->pid == pid)
		return true;
	else
		return false;
}

t_list * getHeadListaPaginas(PIDPAGINAS * data){

	if (data->headListaDePaginas == NULL)
		data->headListaDePaginas = list_create();

	return data->headListaDePaginas;
}

PAGINA * obtenerPaginaDeTablaDePaginas(t_list *headerTablaPaginas, int nroPagina) {

	t_link_element *aux = headerTablaPaginas->head;

	while (aux != NULL) {
		if (comparaNroDePagina(aux->data, nroPagina))
			return aux->data;

		aux = aux->next;
	}

	return NULL;
}

bool comparaNroDePagina(PAGINA *pagina, int nroPagina) {

	if (pagina->nroPagina == nroPagina)
		return true;
	else
		return false;
}

void swapUpdate(void){
// 1+CANTIDAD_DE_PAGINAS_LIBRES ( 5 bytes )

	char package[5];

	if ( recv(socketClienteSwap, (void*) package, SIZE_HANDSHAKE_SWAP, 0) > 0 ){
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
	char trama_handshake[SIZE_HANDSHAKE_SWAP];
	buffer = (char * )malloc (SIZE_HANDSHAKE_SWAP);

	sprintf(buffer,"U%04d",umcGlobalParameters.marcosSize);

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
void pedidoBytes(int *socketBuff, int *pid_actual){

	int 	_pagina,
			_offset,
			_tamanio,
			tamanioContenidoPagina,
			indice_buff;
	char 	buffer[4];
	t_list * headerTablaDePaginas = NULL;
	PAGINA 	*aux = NULL,
			*temp = NULL;
	void *contenidoPagina  = NULL;

	// levanto nro de pagina
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_pagina=atoi(buffer);
	// levanto offset
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_offset=atoi(buffer);
	// levanto tamanio
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_tamanio=atoi(buffer);

	if((umcGlobalParameters.entradasTLB > 0 ) && (consultarTLB(pid_actual,_pagina,&indice_buff))){	// ¿Se usa TLB ? y.. ¿Esta en TLB?
		// si esta en la tabla,  ya tengo el indice y de ahi me voy a vectorMarcos[indice]

		temp = (PAGINA *) malloc (sizeof(PAGINA));
		temp->nroDeMarco = indice_buff;
		resolverEnMP(socketBuff,temp,_offset,_tamanio);	// lo hago asi para poder reutilizar resolverEnMP , que al fin y al cabo es lo que se termina haciendo
		free(temp);

	}
	else {	// No esta en TLB o no se usa TLB

		headerTablaDePaginas = obtenerTablaDePaginasDePID(headerListaDePids, *pid_actual);

		aux = obtenerPaginaDeTablaDePaginas(headerTablaDePaginas, _pagina);

		if (aux == NULL) {	// valido pedido de pagina
			pedidoDePaginaInvalida(socketBuff);
		}
		else {

			if (aux->presencia == AUSENTE) {    // La pagina NO se encuentra en memoria principal

				contenidoPagina = pedirPaginaSwap(socketBuff, pid_actual, aux->nroPagina, &tamanioContenidoPagina); //1- Pedir pagina a Swap

				if ( marcosDisponiblesEnMP() == 0){	// NO HAY MARCOS LIBRES EN MEMORIA
					if( cantPagDisponiblesxPID(pid_actual) == umcGlobalParameters.marcosXProc ){// si el proceso no tiene asignado ningun marco
						// SE ABORTA EL PROCESO
					}
					else{ // no hay marcos disponibles y el proceso tiene asignado por lo menos 1 marco en memoria x lo que se aplica algoritmo
						algoritmoClock(*pid_actual,_pagina,tamanioContenidoPagina,contenidoPagina);
					}
				}else{		// hay marcos disponibles

					if  ( cantPagDisponiblesxPID(pid_actual) > 0){ // SI EL PROCESO TIENE MARGEN PARA ALMACENAR MAS PAGINAS EN MEMORIA
						almacenoPaginaEnMP(pid_actual, aux->nroPagina, contenidoPagina, tamanioContenidoPagina);
                        //TODO(Alan): llamado directo, fix temporal.
                        resolverEnMP(socketBuff, aux, _offset, _tamanio);
					}
					else{	// el proceso llego a la maxima cantidad de marcos proceso
						algoritmoClock(*pid_actual,_pagina,tamanioContenidoPagina,contenidoPagina);
					}
				}
			}
			else { // La pagina se encuentra en memoria principal
				resolverEnMP(socketBuff, aux, _offset, _tamanio); // pagina en memoria principal , se la mando de una al CPU :)
			}
		}
	}
}
void AlmacenarBytes(int *socketBuff, int *pid_actual) {
// PAGINA+OFFSET+TAMANIO+BYTES
	int 	_pagina,
			_offset,
			_tamanio,
			tamanioContenidoPagina,
			indice_buff;
	char 	buffer[4];
	t_list * headerTablaDePaginas = NULL;
	PAGINA 	*aux = NULL,
			*temp = NULL;
	void 	*bytesAlmacenar   = NULL,
			*contenidoPagina  = NULL;

	// levanto nro de pagina
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_pagina=atoi(buffer);
	// levanto offset
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_offset=atoi(buffer);

	// levanto tamanio
	if(recv(*socketBuff, (void*) buffer, 4, 0) <= 0 )
		perror("recv");

	_tamanio=atoi(buffer);

	// levanto bytes a almacenar

	bytesAlmacenar = (void * ) malloc (_tamanio);

	if(recv(*socketBuff, (void*) bytesAlmacenar, _tamanio, 0) <= 0 )
		perror("recv");

	headerTablaDePaginas = obtenerTablaDePaginasDePID(headerListaDePids, *pid_actual);

	aux = obtenerPaginaDeTablaDePaginas(headerTablaDePaginas, _pagina);

	if (aux == NULL) {	// valido pedido de pagina
		pedidoDePaginaInvalida(socketBuff);
	}
	else {

		if (aux->presencia == AUSENTE) {    // La pagina NO se encuentra en memoria principal

			contenidoPagina = pedirPaginaSwap(socketBuff, pid_actual, aux->nroPagina, &tamanioContenidoPagina); //1- Pedir pagina a Swap

			if ( marcosDisponiblesEnMP() == 0){	// NO HAY MARCOS LIBRES EN MEMORIA
				if( cantPagDisponiblesxPID(pid_actual) == umcGlobalParameters.marcosXProc ){// si el proceso no tiene asignado ningun marco
					// SE ABORTA EL PROCESO
				}
				else{ // no hay marcos disponibles y el proceso tiene asignado por lo menos 1 marco en memoria x lo que se aplica algoritmo , y una vez que ya se trajo la pagina a MP,  ahi si guardo los bytes
					algoritmoClock(*pid_actual,_pagina,tamanioContenidoPagina,contenidoPagina);
					guardarBytesEnPagina(pid_actual, _pagina, _offset, _tamanio, bytesAlmacenar);
					setBitDeUso(*pid_actual,_pagina,1);
					EnviarOKACPU(*socketBuff,*pid_actual);
				}
			}else{		// hay marcos disponibles

				if  ( cantPagDisponiblesxPID(pid_actual) > 0){ // SI EL PROCESO TIENE MARGEN PARA ALMACENAR MAS PAGINAS EN MEMORIA
					almacenoPaginaEnMP(pid_actual, aux->nroPagina, contenidoPagina, tamanioContenidoPagina);
					guardarBytesEnPagina(pid_actual, _pagina, _offset, _tamanio, bytesAlmacenar);
					setBitDeUso(*pid_actual,_pagina,1);
					EnviarOKACPU(*socketBuff,*pid_actual);
				}
				else{	// el proceso llego a la maxima cantidad de marcos proceso
					algoritmoClock(*pid_actual,_pagina,tamanioContenidoPagina,contenidoPagina);
					guardarBytesEnPagina(pid_actual, _pagina, _offset, _tamanio, bytesAlmacenar);
					setBitDeUso(*pid_actual,_pagina,1);
					EnviarOKACPU(*socketBuff,*pid_actual);
				}
			}
		}
		else { // La pagina se encuentra en memoria principal , almaceno de una , seteo bit de modificacion y bit de uso = 1
			guardarBytesEnPagina(pid_actual, _pagina, _offset, _tamanio, bytesAlmacenar);
			setBitDeUso(*pid_actual,_pagina,1);
			setBitModificado(*pid_actual,_pagina,1);
			EnviarOKACPU(*socketBuff,*pid_actual);

		}
	}

}

void EnviarOKACPU(int socketBuff, int pPid) {

	if ( send(socketBuff,"1",1,0) <= 0)
		perror("send");

}

void setBitModificado(int pPid, int pagina, int valorBitModificado) {

	t_list * aux = NULL;
	CLOCK_PAGINA * paginaClock = NULL;

	aux = obtenerHeaderFifoxPid(headerFIFOxPID,pPid);

	paginaClock = obtenerPaginaDeFifo(aux, pagina);

	paginaClock->bitDeModificado = valorBitModificado;


}

void guardarBytesEnPagina(int *pid_actual, int pagina, int offset, int tamanio, void *bytesAGuardar) {

	PAGINA *auxPag = NULL;

	auxPag = obtenerPagina(*pid_actual,pagina);

	memcpy(vectorMarcos[auxPag->nroDeMarco].comienzoMarco+offset,bytesAGuardar,tamanio);

}

// Para tener que aplicar el algoritmo , la situacion es que hay al menos 1 pagina en memoria principal , y necesito seleccionar alguno de los marcos asignados a ese PID para reemplazar
void algoritmoClock(int pPid, int numPagNueva, int tamanioContenidoPagina, void *contenidoPagina) {

	t_list *fifoPID = NULL;
	FIFO_INDICE *PunteroPIDClock = NULL;
	t_link_element *recorredor = NULL;
	int i=0;
	PAGINA *pagina_victima = NULL,
	       *pagina_nueva   = NULL;
	void *bufferContenidoPagina = NULL;



	fifoPID = obtenerHeaderFifoxPid(headerFIFOxPID, pPid);

	PunteroPIDClock = obtenerPunteroClockxPID(headerPunterosClock, pPid);        // levanto el indice que me indica por donde iniciar la barrida de marcos para el algoritmo de reemplazo

	recorredor = list_get_nodo(fifoPID,PunteroPIDClock->indice);		// posiciono el puntero donde quedo la ultima vez que tuve que utilizar el algoritmo


		for (i=PunteroPIDClock->indice;i<(fifoPID->elements_count) && recorredor != NULL;i++,recorredor=recorredor->next){

				if (((CLOCK_PAGINA *)recorredor->data)->bitDeUso == 1){
					((CLOCK_PAGINA *)recorredor->data)->bitDeUso =0;		// pongo en 0 y sigo recorriendo
				}else if (((CLOCK_PAGINA *)recorredor->data)->bitDeUso == 0){    // encontre la victima
					// Reemplazar pagina en FIFO ( seria cambiar el campo bit de uso y nro de pagina), en MEMORIA PRINCIPAL, SETEAR BIT DE PRESENCIA EN TANTO PAGINA NUEVA COMO VIEJA, SETEAR CAMPO NUMERO DE MARCO
					// EN CASO DE TENER ESTAR MODIFICADA LA PAGINA VIEJA, ENVIARLA A SWAP

					pagina_victima = obtenerPagina(pPid, ((CLOCK_PAGINA *) recorredor->data)->nroPagina);

					if (pagina_victima->modificado == 1)    // pagina modificada, enviarla a swap antes de reemplazarla
						PedidoPaginaASwap(pPid,numPagNueva,ESCRITURA);
					// Actualizaciones sobre la tabla de paginas ( actualizo campo nro de marco y bit de presencia )
					setBitDePresencia(pPid,pagina_victima->nroPagina,AUSENTE);
					pagina_nueva = obtenerPagina(pPid,numPagNueva);
					pagina_nueva->nroDeMarco = pagina_victima->nroDeMarco ;
					setBitDePresencia(pPid,numPagNueva,PRESENTE_MP);

					bufferContenidoPagina = calloc(umcGlobalParameters.marcosSize,1);	// hago un calloc cosa de inicializar todo en \0 , puede que el tamaño de la pagina que venga a reemplazar NO ocupe todo el espacio asignado
					memcpy(bufferContenidoPagina,contenidoPagina,tamanioContenidoPagina);	// reemplazo en memoria principal :)
					// Reemplazo en memoria principal
					memcpy(vectorMarcos[pagina_nueva->nroDeMarco].comienzoMarco,bufferContenidoPagina,umcGlobalParameters.marcosSize);		// reemplace pagina
					// Actualizo el nodo de la FIFO ( bit de uso + nro de pagina )
					((CLOCK_PAGINA *)recorredor->data)->bitDeUso =1;
					((CLOCK_PAGINA *)recorredor->data)->nroPagina=pagina_nueva->nroPagina;

					PunteroPIDClock->indice++;	// actualizo el indice pa la proxima vez que se ejecute el CLOCK

					break;
				}

			if ( i == (fifoPID->elements_count) ){	// si ya llegue al final de la fifo,  vuelvo al COMIENZO
				i = 0;
				recorredor = fifoPID->head;
			}

		}


}

void PedidoPaginaASwap(int pid, int pagina, int operacion) {

	char *trama;
	PAGINA *pag = NULL;
	pag = obtenerPagina(pid,pagina);
	int trama_size = umcGlobalParameters.marcosSize +((sizeof(int)) * 2)+ 1;

	trama = (char *) malloc(trama_size);

	if (operacion == ESCRITURA){
		sprintf(trama,"%d%04d%04d",ESCRITURA,pid,pagina);
	}else if ( operacion == LECTURA){
		sprintf(trama,"%d%04d%04d",LECTURA,pid,pagina);
	}
	memcpy((void * )&trama[9],vectorMarcos[pag->nroDeMarco].comienzoMarco,umcGlobalParameters.marcosSize);
	enviarPaginaAlSwap(trama,trama_size);
}

static t_link_element* list_get_element(t_list* self, int index) {
	int cont = 0;

	if ((self->elements_count > index) && (index >= 0)) {
		t_link_element *element = self->head;
		while (cont < index) {
			element = element->next;
			cont++;
		}
		return element;
	}
	return NULL;
}



t_link_element * list_get_nodo(t_list *self, int index) {
	t_link_element* element_in_index = NULL;
	element_in_index = list_get_element(self, index);
	return element_in_index != NULL ? element_in_index: NULL;
}

FIFO_INDICE * obtenerPunteroClockxPID(t_list *clock, int pid) {

	t_link_element * aux = NULL;

	aux  = clock->head;

	while (aux != NULL){
		if (((FIFO_INDICE*)aux->data)->pid == pid )
			return ((FIFO_INDICE*)aux->data);

		aux = aux->next;
	}

	return NULL;

}


bool filtrarPorBitDePresencia(void *data){

	if(((PAGINA *) data)->presencia == 1 )
		return true;
	else
		return false;
}

t_link_element *obtenerIndiceTLB(int *pPid, int pagina) {

	t_link_element *aux = NULL;

	aux = headerTLB->head;

	while ( aux != NULL){
		if ( (((TLB *)aux->data)->pid == *pPid) && (((TLB *)aux->data)->nroPagina == pagina) )
			return aux;
		aux = aux->next;
	}

}
bool consultarTLB(int *pPid, int pagina, int *indice) {

	t_link_element *aux = NULL;

	aux = obtenerIndiceTLB(pPid,pagina);
	if ( aux == NULL)
		return false;
	else {
		*indice = ((TLB *) aux->data)->indice;
		return true;
	}
}

void *pedirPaginaSwap(int *socketBuff, int *pid_actual, int nroPagina, int *tamanioContenidoPagina) {

	// Le pido al swap la pagina : 3+pid+nroPagina
	char 	buffer[9],
			buff_pid[4];
	void *	contenidoPagina;
	int 	__pid;

	sprintf(buffer, "2%04d%04d", *pid_actual, nroPagina);    // PIDO PAGINA

	// while(recv(swap) > 0 :

	if (send(socketClienteSwap, buffer, 9, 0) <= 0)
		perror("send");

	// respuesta swap : pid+PAGINA

	if (recv(socketClienteSwap, (void *) buff_pid, 4, 0) <= 0)    // SWAP ME DEVUELVE LA PAGINA
		perror("recv");

	__pid = atoi(buff_pid);


	if (__pid == *pid_actual) {    // valido que la pagina que me respondio swap se corresponda a la que pedi

		contenidoPagina = (void *) malloc(sizeof(umcGlobalParameters.marcosSize));

		if ((*tamanioContenidoPagina = recv(socketClienteSwap, contenidoPagina, umcGlobalParameters.marcosSize, 0)) <= 0)
			perror("recv");

		return contenidoPagina;
	}
	return NULL;
}

// Funcion que me dice cuantas marcos mas puede utilizar el proceso en memoria
int cantPagDisponiblesxPID(int *pPid) {

	t_link_element *aux = NULL;

	aux = obtenerPidPagina(*pPid);

	return (umcGlobalParameters.marcosXProc - getcantPagEnMP(aux->data));

}

t_link_element *obtenerPidPagina(int pPid) {

	t_link_element *aux = NULL;
	aux = headerListaDePids->head;

	while(aux != NULL){
		if ( (compararPid(aux->data,pPid)) )
			return aux;
		aux = aux->next;
	}

}


int marcosDisponiblesEnMP() {
	int contador=0,i=0;

	for(i=0;i<umcGlobalParameters.marcos;i++){
		if ( vectorMarcos[i].estado == LIBRE)
			contador++;
	}
	return  contador;
}

void almacenoPaginaEnMP(int *pPid, int pPagina, char codigo[], int tamanioPagina) {

	void *comienzoPaginaLibre = NULL;
	int indice;
	CLOCK_PAGINA *pagina;
	CLOCK_PID * pid_clock  = NULL;
	FIFO_INDICE *nuevoPidFifoIndice;
	t_list * headerFifo = NULL;

	pagina = (CLOCK_PAGINA *) malloc(sizeof(CLOCK_PAGINA));
	pagina->bitDeModificado = 0;
	pagina->bitDeUso = 1;
	pagina->nroPagina = pPagina;

	pid_clock = ( CLOCK_PID *) malloc ( sizeof(CLOCK_PID)) ;
	pid_clock->pid = *pPid;
	pid_clock->headerFifo = NULL;

	nuevoPidFifoIndice = (FIFO_INDICE *) malloc (sizeof(FIFO_INDICE));
	nuevoPidFifoIndice->pid = *pPid;
	nuevoPidFifoIndice->indice=0;

	if ( headerPunterosClock == NULL)		// si todavia no hay ningun marco en memoria , creo la lista de donde me quedo el puntero en el ultimo reemplazo
		headerPunterosClock = list_create();
	else if( pidEstaEnListaDeUltRemp(headerPunterosClock,*pPid) == false){		//  si el pid todavia no esta en la lista de ultimo puntero, entonces lo agrego
		list_add(headerPunterosClock,nuevoPidFifoIndice);
	}

	if ( headerFIFOxPID == NULL) {        // si es el primer marco que se va a cargar en memoria, entonces creo la lista de pids en memoria
		headerFIFOxPID = list_create();
		list_add(headerFIFOxPID, pid_clock);
	} else if ( pidEstaEnListaFIFOxPID(headerFIFOxPID,pid_clock->pid) == false){		// si el pid  todavia no fue cargado a la lista ( pq no tiene ningun marco en memoria)
		list_add(headerFIFOxPID,pid_clock);
	}

    //TODO(Alan): Esto esta retornando un puntero a null, hay que retornar la estructura que lo contiene para inicializarlo.
//	if ( (headerFifo = obtenerHeaderFifoxPid(headerFIFOxPID,*pPid)) == NULL) // si el PID todavia no tiene ningun marco asignado en memoria , creo la FIFO asociada a ese PID
//		headerFifo = list_create();
    CLOCK_PID * entry = NULL;
    entry = obtenerCurrentClockPidEntry(headerFIFOxPID,*pPid);
         if(entry->headerFifo == NULL) {  // si el PID todavia no tiene ningun marco asignado en memoria , creo la FIFO asociada a ese PID
            entry->headerFifo = list_create();
         }


	list_add(entry->headerFifo, pagina);	// agrego la pagina a la fifo (  se agregan al final de la lista)


	comienzoPaginaLibre = obtenerMarcoLibreEnMP(&indice);	// levanto el primer MARCO LIBRE en MP

	// almaceno pagina en memoria principal
	memcpy(comienzoPaginaLibre,codigo,tamanioPagina);
	// actualizo bit de presencia y marco donde se encuentra almacenado en memoria principal
	setBitDePresencia(*pPid,pPagina,PRESENTE_MP);
	setNroDeMarco(pPid,pPagina,indice);
	// actualizo el valor cantPagEnMP asignado a ese PID
	setcantPagEnMPxPID(*pPid,( getcantPagEnMPxPID(*pPid) + 1 ) );		// en realidad aca podria usar el campo count del headerFifo
	// actualizo bit de uso asociado al marco
	setBitDeUso(*pPid,pPagina,1);

}

bool pidEstaEnListaDeUltRemp(t_list *headerPunteros, int pPid) {

	t_link_element * aux = NULL;
	aux = headerPunteros->head ;
	while ( aux != NULL){

		if ((((FIFO_INDICE*)aux->data)->pid) == pPid )
			return true;

		aux = aux->next;
	}
	return false;
}

bool pidEstaEnListaFIFOxPID(t_list *headerFifos, int pPid) {
	t_link_element * aux = NULL;
	aux = headerFifos->head ;
	while ( aux != NULL){
		if (((CLOCK_PID *)aux->data)->pid == pPid )
			return true;
		aux = aux->next ;
	}
	return false;
}

t_list *obtenerHeaderFifoxPid(t_list *headerFifos, int pPid) {
	t_link_element * aux  = NULL;
	aux = headerFifos->head ;
	while ( aux != NULL){
		if (((CLOCK_PID *) aux->data)->pid == pPid )
			return ((CLOCK_PID *) aux->data)->headerFifo ;
		aux = aux->next;
	}
	return NULL;
}

CLOCK_PID * obtenerCurrentClockPidEntry(t_list * headerFifos, int pid) {
	t_link_element * aux  = NULL;
	aux = headerFifos->head ;
	while ( aux != NULL){
		if (((CLOCK_PID *) aux->data)->pid == pid )
			return ((CLOCK_PID *) aux->data);
		aux = aux->next;
	}
	return NULL;
}
void setBitDeUso(int pPid, int pPagina, int valorBitDeUso) {

	t_list *fifoDePid = NULL;
	CLOCK_PAGINA *pagina = NULL;
	//PAGINA * pagina = NULL;
	//tablaDePaginas = obtenerTablaDePaginasDePID(headerListaDePids, pPid);
	//pagina = obtenerPaginaDeTablaDePaginas(tablaDePaginas, pPagina);
	//vectorMarcos[pagina->nroDeMarco].bitDeUso = (unsigned char ) valorBitDeUso;

	fifoDePid = obtenerHeaderFifoxPid(headerFIFOxPID,pPid);

	pagina = obtenerPaginaDeFifo(fifoDePid,pPagina);

	pagina->bitDeUso=valorBitDeUso;

}

CLOCK_PAGINA * obtenerPaginaDeFifo(t_list *fifoDePid, int pagina) {

	t_link_element *aux = NULL;

	aux = fifoDePid->head;

	while ( aux != NULL){
		if ( ((CLOCK_PAGINA *)aux->data)->nroPagina == pagina )
			return ((CLOCK_PAGINA *)aux->data);
		aux = aux->next;
	}

}

void setcantPagEnMPxPID(int pPid, int nuevocantPagEnMP) {

	t_link_element *aux = NULL;
	aux = obtenerPidPagina(pPid);
	((PIDPAGINAS *)aux->data)->cantPagEnMP = nuevocantPagEnMP ;

}

int getcantPagEnMPxPID(int pPid) {
	t_link_element *aux = NULL;
	aux = obtenerPidPagina(pPid);
	return getcantPagEnMP(aux->data);
}

int getcantPagEnMP(PIDPAGINAS *pPidPagina) {
	return pPidPagina->cantPagEnMP;
}


void setNroDeMarco(int *pPid, int pPagina, int indice) {

	PAGINA *auxiliar = NULL;

	auxiliar = obtenerPagina(*pPid,pPagina);

	auxiliar->nroDeMarco = indice;

}

void setBitDePresencia(int pPid, int pPagina, int estado) {

	PAGINA *auxiliar = NULL;

	auxiliar = obtenerPagina(pPid,pPagina);

	auxiliar->presencia = estado;

}

PAGINA *obtenerPagina(int pPid, int pPagina) {

	t_list *headerTablaDePaginas;
	PAGINA *pag_aux = NULL;

	headerTablaDePaginas = obtenerTablaDePaginasDePID(headerListaDePids,pPid);

	pag_aux = obtenerPaginaDeTablaDePaginas(headerTablaDePaginas,pPagina);

	return pag_aux;

}


void *obtenerMarcoLibreEnMP(int *indice) {

	int i = 0;

	while(i<umcGlobalParameters.marcos){
		*indice = i;
		if ( vectorMarcos[i].estado == LIBRE)
			return vectorMarcos[i].comienzoMarco;
		i++;
	}
	return NULL;
}

void resolverEnMP(int *socketBuff, PAGINA *pPagina, int offset, int tamanio) {
// 1+CODIGO
	char buffer[tamanio+1];
	buffer[0]='1';

	memcpy(buffer,(vectorMarcos[pPagina->nroDeMarco].comienzoMarco)+offset,tamanio);

	if (send(*socketBuff,buffer,tamanio,0) <= 0)	// envio a CPU la pagina
		perror("send");

}

void pedidoDePaginaInvalida(int *socketBuff) {

	if ( send(*socketBuff,"2",1,0) == -1 )
		perror("send");
}


//Funciones para el Manejo de TLB

void limpiarPidDeTLB(t_list * Tlb , int pPid  ){

	t_link_element *aux = NULL;

	aux = Tlb->head;

	while(aux != NULL){
		if ( ((TLB *)aux->data)->pid == pPid )
			((TLB *)aux->data)->estado = LIBRE;

		aux =  aux->next;

	}


}

void agregarPaginaATLB(t_list *Tlb, int pPid, PAGINA *pPagina){


	t_link_element *aux = NULL;

	aux = Tlb->head;

	while (aux != NULL){	// barro toda la TLB hasta encontrar una entrada LIBRE, y ahi nomas reemplazo :)
		if( ((TLB*)aux->data)->estado == LIBRE){
			((TLB*)aux->data)->pid = pPid;
			((TLB*)aux->data)->nroPagina=pPagina->nroPagina;
			((TLB*)aux->data)->estado=OCUPADO;
			break;
		}
		aux = aux->next;
	}

}


