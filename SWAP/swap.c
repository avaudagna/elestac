#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

//Librerias propias
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include "socketCommons.h"


//OPERACIONES UMC
#define ESCRIBIR 1
#define LECTURA 2
#define FINALIZARPROG 3


/************************
 * VARIABLES GLOBALES
 ***********************/

//SWAP
char* ABSOLUTE_PATH_SWAP;
char* PATH_LOG;
char* PATH_SWAP_FILE;
char* PATH_CONF;

char* bitMap;
t_bitarray* bitArrayStruct;
int SWAP_BLOCKSIZE;
t_log* LOG_SWAP;
int TAMANIO_PAGINA;

struct InformacionPagina {
   int pid;
   int pageNumber;
   int positionInSWAP;
   int bitMapPosition;
};

typedef struct NodoControlCodigo {
    struct InformacionPagina * infoPagina;
    struct NodoControlCodigo * next;
} controlCodigo_t;

struct NodoControlCodigo* headControlCodigo = NULL;

//CONEXION
int swapSocket;
int umcSocket;
char* package;

//CONFIG
char* IP_SWAP;
char* PUERTO_SWAP;
int CANTIDAD_PAGINAS;
int RETARDO_COMPACTACION;
char* SWAP_DATA_NAME;
int RETARDO_ACCESO;

/**********************
 *
 * 		FUNCIONES
 *
 **********************/

//PEDIDOS UMC
int umc_handshake();

char* request_Lectura(int pid, int numeroPagina);
void umc_leer();

int request_EscrituraPagina(int pid, int numeroPagina, char* codigo);
void umc_escribir();

int request_FinalizacionPrograma(int pid);
void umc_finalizarPrograma();

//Inicializacion y finalizacion
int init_args(int argc, char **argv);
int init_Config();
void init_Server();
int init_SwapFile();
int init_BitMap();
void close_SwapProcess();

//Procesos SWAP
int swap_EspacioDisponible(int cantPaginasNecesarias);
int swap_PrimerOcupado(int principio);
int swap_Compactar();
int swap_LiberarPrograma(int pidDelProceso);
char* swap_ObtenerPagina(int pidDelProceso, int numeroPagina);

//Manejo de paginas
int paginas_CantidadPaginasNecesarias(char* codigo);
int paginas_EscribirPaginaEnSWAP(char* contenidoAEscribir, int posicion);
void paginas_MoverPagina(int bitOrigen, int bitDestino);
char* paginas_LeerPagina(int posicion);
int paginas_CalcularPosicionEnSWAP(int numeroPaginaBitMap);
int paginas_CantidadPaginasLibres();

//Estructura Control
int listaControl_IniciarLista(struct InformacionPagina infoBloque);
void listaControl_AgregarNodo(int pid, int pageNumber, int positionInSWAP, int bitMapPosition);
int listaControl_ObtenerPosicionEnSWAP(int pid, int pageNumber);
struct InformacionPagina listaControl_ObtenerNodoPorBitMap(int bitMap);
int listaControl_Longitud();
int listaControl_EliminarNodo(int pidDelProceso, int pageNumberDelProceso);
int listaControl_EliminarNodoIndex(int index);
void listaControl_EliminarLista();

//Imprimir
void imprimir_EstadoBitMap();
void imprimir_NodosEstructuraControl();

//Otras
char obtenerPrimerChar(void* buffer);
int mod (int a, int b);
void excepcionAlHablarConUMC();
void dormirse(int segundos);

int main (int argc, char **argv){

	if(init_args(argc, argv)){
		return -1;
	}

	LOG_SWAP = log_create(PATH_LOG, "Elestac-SWAP", true, LOG_LEVEL_TRACE);
	printf("[INFO] Log created in: %s \n", PATH_LOG);

	log_info(LOG_SWAP, ".:: INITIALIZING SWAP ::.");
	if(init_Config()){
		log_error(LOG_SWAP, "Config file can not be loaded");
    	return -1;
    }

    puts("");

    init_Server();

    close_SwapProcess();

	return 0;
}


/**********************
 *
 *		PEDIDOS UMC
 *
 **********************/
int umc_handshake(){
	int handshakeOK = 0;
	char handShake;
	int tamanioTrama = sizeof(char)+sizeof(int);

	void* buffer = malloc(sizeof(int));

    //recv el identificador de operacion (espero una 'U')
	if(recv(umcSocket, &handShake, sizeof(char),0)<= 0){
		free(buffer);
		return -1;
	}

	if(handShake == 'U'){ //El handshake es correcto

        //recv el tamanio de pagina
        if(recv(umcSocket, buffer,sizeof(int),0)<= 0){
            free(buffer);
            return -1;
        }

        TAMANIO_PAGINA = atoi(buffer);

		init_BitMap();

		//Armo la trama a responder char[]: S+cantPaginasLibres
		int SIZE_TRAMA = 5;
		char* respuesta = malloc(SIZE_TRAMA);
		sprintf(respuesta,"S%04d", paginas_CantidadPaginasLibres());

		if(send(umcSocket,(void *) respuesta, SIZE_TRAMA,0) == -1){
			log_error(LOG_SWAP, "Error while trying to complete handshake");
			return handshakeOK;
		}

        free(respuesta);

		log_info(LOG_SWAP, "Handshake with UMC succesfully done. Page size received: %d", TAMANIO_PAGINA);
		handshakeOK = 1;

		if(!init_SwapFile()){
			log_error(LOG_SWAP, "SWAP file couldn't be created. Check the conf file!");
			close_SwapProcess();
		}

	} else {
		char tramaRecibida[100];
		memcpy(&tramaRecibida, buffer, tamanioTrama);

		log_error(LOG_SWAP, "Error while trying to complete handshake. Code recieved: %s", tramaRecibida);
	}

	free(buffer);
	return handshakeOK;
}

char* request_Lectura(int pid, int numeroPagina){
	log_info(LOG_SWAP, "[REQUEST] Page requested by UMC [PID: %d | Page number: %d] \n", pid, numeroPagina);

	dormirse(RETARDO_ACCESO); //Aplicamos el retraso de acceso

	char* paginaObtenida = NULL;
	paginaObtenida = swap_ObtenerPagina(pid, numeroPagina);

	if(paginaObtenida == NULL){
		log_warning(LOG_SWAP, "An error occurred while trying to fetch the page [PID: %d | Page number: %d]", pid, numeroPagina);
		return NULL;
	}

	log_info(LOG_SWAP, "Page found");

	return paginaObtenida;
}

int request_EscrituraPagina(int pid, int numeroPagina, char* codigo){
	log_info(LOG_SWAP, "[REQUEST] Page writing requested by UMC [PID: %d | Page number: %d] \n", pid, numeroPagina);

	dormirse(RETARDO_ACCESO); //Aplicamos el retraso de acceso

	char* paginaObtenida = NULL;
	paginaObtenida = swap_ObtenerPagina(pid, numeroPagina);

	imprimir_EstadoBitMap();

	if(paginaObtenida != NULL){ //Si la pagina ya existe, la sobreescribo
		log_info(LOG_SWAP, "Requested page already exists. Overwriting existing page");

		int posicionPaginaEnSWAP;
		posicionPaginaEnSWAP = listaControl_ObtenerPosicionEnSWAP(pid, numeroPagina);

		if(posicionPaginaEnSWAP < 0){
			log_error(LOG_SWAP, "An error occurred while trying to fetch the page");

			return -1;
		}

		paginas_EscribirPaginaEnSWAP(codigo, posicionPaginaEnSWAP);
		log_info(LOG_SWAP, "Requested page has been successfully overwritten");

		return 0;
	}

	int bitMapPosition = swap_EspacioDisponible(1); //Solo necesitamos escribir una pagina

	int posicionEnSWAP = paginas_CalcularPosicionEnSWAP(bitMapPosition);

	int escritoEnPosicion = paginas_EscribirPaginaEnSWAP(codigo, posicionEnSWAP);

	if(escritoEnPosicion < 0){
		return -2;
	}

	listaControl_AgregarNodo(pid, numeroPagina, posicionEnSWAP, bitMapPosition);
	bitarray_set_bit(bitArrayStruct, bitMapPosition);

	log_info(LOG_SWAP, "Requested page has been successfully written");

	imprimir_EstadoBitMap(); //SOLO IMPRIMO EL BITMAP CUANDO ESCRIBO UNA NUEVA PAGINA

	return 0;
}

int request_FinalizacionPrograma(int pid){
	log_info(LOG_SWAP, "[REQUEST] Program finalization requested by UMC [PID: %d] \n", pid);

	imprimir_EstadoBitMap();

	int paginasLiberadas = 0;
	paginasLiberadas = swap_LiberarPrograma(pid);

	if(paginasLiberadas > 0){
		log_info(LOG_SWAP, "Program finalized. Pages released: %d", paginasLiberadas);

		imprimir_EstadoBitMap();

		return 1;
	} else {
		log_error(LOG_SWAP, "Error while releasing pages");

		return -1;
	}

}

/**********************
 *
 *	INICIALIZACION
 *
 **********************/

//TODO
int init_args(int argc, char **argv){

	if(argc < 0){
		puts("[FATAL ERROR] Wrong number of parameters. Expected parameters: ConfigName");

		return -1;
	}

	//Por parametro recibo el nombre del archivo de configuracion
	//Obtenemos el path donde se encuentra la aplicacion
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	ABSOLUTE_PATH_SWAP = malloc(sizeof(cwd));
	strcpy(ABSOLUTE_PATH_SWAP, cwd);

	//Especificamos el path del log
	PATH_LOG = malloc(sizeof(char)*150);
	strcat(PATH_LOG, ABSOLUTE_PATH_SWAP);
	strcat(PATH_LOG, "/swap.log");
	printf("[INFO] PATH LOG: %s \n", PATH_LOG);

	//Especificamos el path del archivo de configuracion
	PATH_CONF = malloc(sizeof(char)*150);
	strcat(PATH_CONF, ABSOLUTE_PATH_SWAP);
	strcat(PATH_CONF, "/");
	strcat(PATH_CONF, argv[1]); //CAMBIAR POR argv[0]
	printf("[INFO] PATH SWAP config: %s \n", PATH_CONF);

	return 0;
}

int init_Config(){
	//ARCHIVO DE CONFIGURACION
    char* keys[6] = {"IP_SWAP",
					 "PUERTO_ESCUCHA",
					 "CANTIDAD_PAGINAS",
					 "RETARDO_COMPACTACION",
					 "SWAP_DATA",
					 "RETARDO_ACCESO"};

	log_info(LOG_SWAP, "Reading configuration File");

	t_config * punteroAStruct = config_create(PATH_CONF);

	if(punteroAStruct != NULL) {
		log_info(LOG_SWAP, "Config file loaded: %s", PATH_CONF);

		int cantKeys = config_keys_amount(punteroAStruct);
		log_info(LOG_SWAP, "Number of keys found: %d", cantKeys);

		int i = 0;
		for ( i = 0; i < cantKeys; i++ ) {
			if (config_has_property(punteroAStruct,keys[i])){
				switch(i) {
					case 0:
						IP_SWAP = config_get_string_value(punteroAStruct,keys[i]);
						log_info(LOG_SWAP, "%s --> %s", keys[i], IP_SWAP);
						break;

					case 1:
						PUERTO_SWAP = config_get_string_value(punteroAStruct,keys[i]);
						log_info(LOG_SWAP, "%s --> %s", keys[i], PUERTO_SWAP);
						break;

					case 2:
						CANTIDAD_PAGINAS = config_get_int_value(punteroAStruct,keys[i]);
						log_info(LOG_SWAP, "%s --> %d", keys[i], CANTIDAD_PAGINAS);
						break;

					case 3:
						RETARDO_COMPACTACION = config_get_int_value(punteroAStruct,keys[i]);
						log_info(LOG_SWAP, "%s --> %d", keys[i], RETARDO_COMPACTACION);
						break;

					case 4:
						SWAP_DATA_NAME = config_get_string_value(punteroAStruct,keys[i]);
						log_info(LOG_SWAP, "%s --> %s", keys[i], SWAP_DATA_NAME);
						break;

					case 5:
						RETARDO_ACCESO = config_get_int_value(punteroAStruct,keys[i]);
						log_info(LOG_SWAP, "%s --> %d", keys[i], RETARDO_ACCESO);
						break;
				}

			}
		}

		free(punteroAStruct);

		return 0;
	} else {
		free(punteroAStruct);

		return 1;
	}
}

void init_Server(){
	log_info(LOG_SWAP, ".:: INITIALIZING SERVER PROCESS ::.");

	setServerSocket(&swapSocket,IP_SWAP,atoi(PUERTO_SWAP));

	acceptConnection(&umcSocket, &swapSocket);

	void* buffer = NULL;
//	int handshakeOK = 0;
	int operacion = 0;


    if(umc_handshake() != 1){
		log_error(LOG_SWAP, "Bad UMC hanshake");
    	close_SwapProcess();
	}

    while(1){
		log_info(LOG_SWAP, "Waiting for UMC request");

		buffer = malloc(sizeof(char));
        if(recv(umcSocket,buffer,sizeof(char),0) <= 0){
            free(buffer);
            close(umcSocket);
            close(swapSocket);

            return;
        }

        /****************************
         * Leo el codigo de operacion
         * y convierto el char a int
         ****************************/
        operacion = atoi(buffer);
        free(buffer);

        switch (operacion) {
            case ESCRIBIR: //pid(int), numeroPagina(int), codigo (undefined)
                umc_escribir();

                break;
            case LECTURA: //pid(int), numeroPagina(int)
                umc_leer();

                break;
            case FINALIZARPROG: //pid(int)
                umc_finalizarPrograma();

                break;
            default:
                log_warning(LOG_SWAP, "Request code unknown: %d \n", operacion);
                break;
        }
    }
//        else { //volvemos a tratar de hacer el hand
//			handshakeOK = umc_handshake();
//
//			char error[1] = {'E'};
//			if(send(umcSocket,(void *) error, 1,0) == -1) {
//				perror("send");
//			}
//
//		}

	}


void umc_leer(){
	int pid;
	int numeroPagina;

	char* buffer = malloc(sizeof(int));

	//Recibo el PID
	if(recv(umcSocket, buffer, sizeof(int), 0)<= 0)
		excepcionAlHablarConUMC();

	//Lo convertimos a int
	pid = atoi(buffer);

	free(buffer);
	buffer = malloc(sizeof(int));

	//Recibo el numero pagina
	if(recv(umcSocket, buffer, sizeof(buffer), 0)<= 0)
		excepcionAlHablarConUMC();

	//Lo convertimos a int
	numeroPagina = atoi(buffer);

	free(buffer);
	buffer = malloc(sizeof(char) * TAMANIO_PAGINA);

	//Hacemos el request
	char* resultadoRequest = request_Lectura(pid, numeroPagina);

	if(resultadoRequest != NULL){ //ENVIAMOS A UMC 1+cantPaginasLibres
		void* respuesta = calloc(1, sizeof(int) + TAMANIO_PAGINA);
		sprintf(respuesta,"%04d%s",pid, resultadoRequest);

		if(send(umcSocket, respuesta, sizeof(int) + TAMANIO_PAGINA,0) == -1)
			excepcionAlHablarConUMC();

		free(respuesta);

	} else { //ENVIAMOS ERROR A UMC
		char error[1] = {'E'};
		if(send(umcSocket,(void *) error, 1,0) == -1)
			excepcionAlHablarConUMC();

	}

	free(buffer);
}

void umc_escribir(){
	int SIZE_TRAMA = 5;

	char* buffer = malloc(sizeof(int));

	int pid;
	int numeroPagina;
	char* codigo = malloc(sizeof(char) * TAMANIO_PAGINA);

	//Recibo el PID
	if(recv(umcSocket, buffer, sizeof(int), 0)<= 0)
		excepcionAlHablarConUMC();

	pid = atoi(buffer);

	free(buffer);
	buffer = malloc(sizeof(int));

	//Recibo el numero pagina
	if(recv(umcSocket, buffer, sizeof(int), 0)<= 0)
		excepcionAlHablarConUMC();

	numeroPagina = atoi(buffer);

	free(buffer);
	buffer = malloc(sizeof(char) * TAMANIO_PAGINA);

	//Recibimos el codigo
	if(recv(umcSocket, buffer, sizeof(char)*TAMANIO_PAGINA, 0)<= 0)
		excepcionAlHablarConUMC();

	memcpy(codigo, buffer, sizeof(char)*TAMANIO_PAGINA);

	//Hacemos el request
	int resultadoRequest = request_EscrituraPagina(pid, numeroPagina, codigo);

	if(resultadoRequest > -1){ //ENVIAMOS A UMC 1+cantPaginasLibres
		char* respuesta;

		respuesta = malloc(sizeof(char)*SIZE_TRAMA);
		sprintf(respuesta,"1%04d", paginas_CantidadPaginasLibres());

		if(send(umcSocket,(void *) respuesta, SIZE_TRAMA,0) == -1)
			excepcionAlHablarConUMC();

		free(respuesta);

	} else { //ENVIAMOS ERROR A UMC
		char error[1] = {'E'};
		if(send(umcSocket,(void *) error, 1,0) == -1)
			excepcionAlHablarConUMC();
	}

	free(buffer);
	free(codigo);

}

void umc_finalizarPrograma(){
	int pid, SIZE_TRAMA = 5;
	char* buffer = malloc(sizeof(int));

	//Recibo el PID
	if(recv(umcSocket, buffer, 1, 0)<= 0)
		excepcionAlHablarConUMC();

	char aux;
	memcpy(&aux,buffer,1);
	//free(buffer);
	pid = aux - '0';

	free(buffer);
	buffer = malloc(sizeof(int));

	//Hacemos el request
	int resultadoRequest = request_FinalizacionPrograma(pid);

	if(resultadoRequest > 0){ //ENVIAMOS A UMC 1+cantPaginasLibres
		char* respuesta = malloc(sizeof(char)*SIZE_TRAMA);
		sprintf(respuesta,"1%d", paginas_CantidadPaginasLibres());

		if(send(umcSocket,(void *) respuesta, SIZE_TRAMA,0) == -1)
			excepcionAlHablarConUMC();

		free(respuesta);

	} else { //ENVIAMOS ERROR A UMC
		char error[1] = {'E'};
		if(send(umcSocket,(void *) error, 1,0) == -1)
			excepcionAlHablarConUMC();

	}

	free(buffer);
}

int init_SwapFile(){
	log_info(LOG_SWAP, ".:: CREATING SWAP FILE ::.");

	//Creamos el path del archivo data con el nombre especificado en el archivo de configuracion
	PATH_SWAP_FILE = calloc(sizeof(char), 150);
	strcat(PATH_SWAP_FILE, ABSOLUTE_PATH_SWAP);
	strcat(PATH_SWAP_FILE, "/");
	strcat(PATH_SWAP_FILE, SWAP_DATA_NAME);
	log_info(LOG_SWAP, "[INFO] PATH SWAP data: %s", PATH_SWAP_FILE);

	char* comandoCrearSwap = string_new();

	//dd syntax ---> dd if=/dev/zero of=foobar count=1024 bs=1024;
	string_append(&comandoCrearSwap, "dd");

	//Utilizo el archivo zero que nos va a dar tantos NULL como necesite
	string_append(&comandoCrearSwap, " if=/dev/zero");

	//El archivo de destino es mi swap a crear
	string_append(&comandoCrearSwap, " of=");
	if(PATH_SWAP_FILE != NULL)
		string_append(&comandoCrearSwap, PATH_SWAP_FILE);
	else
		string_append(&comandoCrearSwap, "/home/utnso/");

	//Especifico el BLOCKSIZE, que seria el total de mi swap
	SWAP_BLOCKSIZE = TAMANIO_PAGINA*CANTIDAD_PAGINAS;
	string_append(&comandoCrearSwap, " bs=");
	string_append(&comandoCrearSwap, string_itoa(SWAP_BLOCKSIZE)); //Tamanio total: tam_pag * cant_pag

	//Con count estipulo la cantidad de bloques
	string_append(&comandoCrearSwap, " count=");
	string_append(&comandoCrearSwap, "1");

	//printf("%s %s \n", "Command executed --->",comandoCrearSwap);

	if(system(comandoCrearSwap)){
		log_error(LOG_SWAP, "SWAP file can not be created");

		free(comandoCrearSwap);
		return 0;
	}else{
		log_info(LOG_SWAP, "SWAP file has been successfully created");
		free(comandoCrearSwap);
	}

	//Creamos puntero al archivo swap
	FILE* swapFile = fopen(PATH_SWAP_FILE,"rb");

	if (!swapFile){
		log_error(LOG_SWAP, "Unable to open swap file");
		return 0;
	}

	//Cerramos el puntero
	fclose(swapFile);

	return 1;
}

int init_BitMap(){
	//Creamos y limpiamos el BitMap
	bitMap = malloc(sizeof(char)*CANTIDAD_PAGINAS);
	bitArrayStruct = bitarray_create(bitMap,CANTIDAD_PAGINAS/8);

	int i; //Inicializamos todos los bits en cero (vacio)
	for (i = 0; i < bitarray_get_max_bit(bitArrayStruct); ++i) {
		bitarray_clean_bit(bitArrayStruct,i);
	}

	return 1;
}

void close_SwapProcess(){
	log_warning(LOG_SWAP, ".:: Terminating SWAP process: Freeing resources... ::.");

	/******************************
	 * Liberamos variables globales
	 ******************************/

	//SWAP
	free(ABSOLUTE_PATH_SWAP);
	free(bitMap);
	free(bitArrayStruct);
	free(headControlCodigo);

	//CONEXION
	free(package);

	//CONFIG
	free(IP_SWAP);
	free(PUERTO_SWAP);
	free(PATH_SWAP_FILE);

	//Eliminamos la lista con todos sus modulos
	listaControl_EliminarLista();
	free(headControlCodigo);

	//Liberamos el BitMap
	bitarray_destroy(bitArrayStruct);

	log_warning(LOG_SWAP, ".:: SWAP Process terminated ::.");
	log_destroy(LOG_SWAP);
}

/**********************
 *
 *	Procesos SWAP
 *
 **********************/

/**********************************************************************************
 * Devuelve la posicion inicial desde donde se empiezas a escribir la o las paginas
 * Compacta si es necesario
 *
 * Devuelve -1 si el espacio disponible es insuficiente
 **********************************************************************************/
int swap_EspacioDisponible(int cantPaginasNecesarias){
	int primerEspacio = 0;
	int aux = 0;
	int lugaresParciales = 0;
	int totalLibres = 0;
	int meEncuentroEn = 0;
	int paginasRestantes = 0;

	while (meEncuentroEn < CANTIDAD_PAGINAS) {
		if(cantPaginasNecesarias > CANTIDAD_PAGINAS) //Verifico que la cantidad de paginas requeridas sea menor a las totales
			return -1;

		primerEspacio = meEncuentroEn;

		if(!bitarray_test_bit(bitArrayStruct,primerEspacio)){ //Si esta disponible me fijo si tiene la cant de pag necesarias
			totalLibres++;
			lugaresParciales = 1;

			paginasRestantes = CANTIDAD_PAGINAS - meEncuentroEn;
			if(paginasRestantes < cantPaginasNecesarias){
				meEncuentroEn++;
				break;
			}

			if(cantPaginasNecesarias == 1){ //Si solo necesito una pagina
				return meEncuentroEn;
			}

			for (aux = primerEspacio; aux < (meEncuentroEn + cantPaginasNecesarias); aux++) { // For por la cantidad de paginas requeridas
				meEncuentroEn++;

				if(bitarray_test_bit(bitArrayStruct,meEncuentroEn)){ //Si esta ocupado salimos
					break;
				}

				lugaresParciales++;
				totalLibres++;

				if(lugaresParciales == cantPaginasNecesarias){
 					return primerEspacio; //Hay lugar disponible y continuo ;)
				}
			}

		} else {
			meEncuentroEn++;
		}

	}

	if(totalLibres >= cantPaginasNecesarias)
		return swap_Compactar(); //Hay lugar pero hay que compactar

	return -1; //No hay lugar :(
}

//Desde la posicion indicada, encuentra el proximo bit ocupado
int swap_PrimerOcupado(int principio){
	int actual = principio + 1;
	int ultimoBit = bitarray_get_max_bit(bitArrayStruct);

	while(!bitarray_test_bit(bitArrayStruct, actual)){
		if(actual == ultimoBit)
			return -1;

		actual++;
	}

	return actual;
}

//TODO: Agregar retardo de compactacion
int swap_Compactar(){
	log_info(LOG_SWAP, ".:: DEFRAGMETATION REQUESTED ::.");

	puts(".:: SWAP STATE BEFORE DEFRAGMETATION ::. \n");
	imprimir_EstadoBitMap();

	puts("\n .:: BEGINING DEFRAGMETATION ::. \n");

	int i = 0;
	for (i = 0; i < 10001; i++){
		usleep(RETARDO_COMPACTACION);
		printf("\r\e[?25l Loading... %d", i/100);
	}

	int espacioActual, espacioLibre = 0;
	while(bitarray_test_bit(bitArrayStruct, espacioLibre)){ //Encontramos el primer espacio libre
		espacioLibre++;
	}

	for(espacioActual = 0; espacioActual < CANTIDAD_PAGINAS; espacioActual++) { //Empezamos a recorrer el BitMap desde el primer elemento
		if(bitarray_test_bit(bitArrayStruct, espacioActual) && espacioActual > espacioLibre){ //Si esta ocupado y es superior a un espacio libre, estramos
			int proximoOcupado = swap_PrimerOcupado(espacioLibre); //Encontramos el espacio ocupado siguiente

			paginas_MoverPagina(proximoOcupado, espacioLibre); //Movemos la pagina
			espacioLibre++; //Marcamos el espacio movido como libre
		}

	}

	puts(".:: SWAP STATE AFTER DEFRAGMETATION ::. \n");
	imprimir_EstadoBitMap();

	return 0;
}

int swap_LiberarPrograma(int pidDelProceso){
	int modificaciones = 0;
	int i = 0;
	struct NodoControlCodigo * current = headControlCodigo;

	while(current != NULL){
		if(current->infoPagina->pid == pidDelProceso){
			bitarray_clean_bit(bitArrayStruct, current->infoPagina->bitMapPosition);
			listaControl_EliminarNodoIndex(i); //CUIDADO

			modificaciones++;
		} else {
			i++;
		}

		current = current->next;
	}

	return modificaciones;
}

char* swap_ObtenerPagina(int pidDelProceso, int numeroPagina){
	int posicionEnSWAP = listaControl_ObtenerPosicionEnSWAP(pidDelProceso,numeroPagina);

	if(posicionEnSWAP > -1){
		return paginas_LeerPagina(posicionEnSWAP);
	}

	return NULL;
}


/**********************
 *
 *	Manejo de paginas
 *
 **********************/
int paginas_CantidadPaginasNecesarias(char* codigo){
	int tamanioCodigo =  string_length(codigo);
	free(codigo);

	if(tamanioCodigo%TAMANIO_PAGINA){
		return (tamanioCodigo/TAMANIO_PAGINA)+1;
	} else {
		return tamanioCodigo/TAMANIO_PAGINA;
	}

}

int paginas_EscribirPaginaEnSWAP(char* contenidoAEscribir, int posicion){
	FILE* fp = fopen(PATH_SWAP_FILE,"r+b");

	char pagina[TAMANIO_PAGINA];
	int i;
	for(i = 0; i < TAMANIO_PAGINA; i++){
		pagina[i] = contenidoAEscribir[i];
	}

	if(!fp){
		log_warning(LOG_SWAP, "Page couldn't be written");
		fclose(fp);
		free(contenidoAEscribir);

		return -1;
	}

	if(fseek(fp,posicion,SEEK_SET)){
		log_warning(LOG_SWAP, "Invalid pointer");
		fclose(fp);
		free(contenidoAEscribir);

		return -1;
	}

	int escritoEnPosicion = ftell(fp);

	//Escribimos, si es distinto al count, fallo
	if(fwrite(pagina,TAMANIO_PAGINA,1,fp) != 1){
		log_warning(LOG_SWAP, "Page couldn't be written");
		fclose(fp);
		free(contenidoAEscribir);

		return -1;
	}

	fclose(fp);
	//free(contenidoAEscribir);

	return escritoEnPosicion;
}

void paginas_MoverPagina(int bitOrigen, int bitDestino){
	//Calculo la posicion destino multiplicando la posicion por el tamanio de pagina
	int posicionDestino = bitDestino*TAMANIO_PAGINA;

	//Obtengo la informacion de la pagina a mover
	struct InformacionPagina nodoObtenido = listaControl_ObtenerNodoPorBitMap(bitOrigen);

	//Copio el contenido de la misma
	char* contenidoPagina = paginas_LeerPagina(nodoObtenido.positionInSWAP);

	//Escribo el contenido de la misma en la posicion del bit destino
	paginas_EscribirPaginaEnSWAP(contenidoPagina, bitDestino*TAMANIO_PAGINA);
	free(contenidoPagina);

	//Elimino el nodo desactualizado
	listaControl_EliminarNodo(nodoObtenido.pid, nodoObtenido.pageNumber);

	//Inserto uno nuevo con la posicion actualizada
	listaControl_AgregarNodo(nodoObtenido.pid, nodoObtenido.pageNumber, posicionDestino, bitDestino);

	//Actualizo el BitMap
	bitarray_set_bit(bitArrayStruct,bitDestino);
	bitarray_clean_bit(bitArrayStruct, bitOrigen);
}

char* paginas_LeerPagina(int posicion){
	FILE* fp = fopen(ABSOLUTE_PATH_SWAP,"rb+");

	if(fp == NULL){
		log_warning(LOG_SWAP, "File couldn't be opened");
		return NULL;
	}

	fseek(fp,posicion,SEEK_SET);

	char* contenidoObtenido = malloc(TAMANIO_PAGINA*sizeof(char)); //CUIDADO ACA

	//Leemos, si es distinto al count, fallo

	if(fread(contenidoObtenido,TAMANIO_PAGINA,1,fp) != 1){
		log_warning(LOG_SWAP, "Page couldn't be readed");
		return NULL;
	}

	fclose(fp);

	return contenidoObtenido;
}

int paginas_CalcularPosicionEnSWAP(int numeroPaginaBitMap){
	return TAMANIO_PAGINA * numeroPaginaBitMap;
}

int paginas_CantidadPaginasLibres(){
	int i, cantidadLibres = 0;

	for (i = 0; i < bitarray_get_max_bit(bitArrayStruct); i++) {
		if(!bitarray_test_bit(bitArrayStruct, i))
			cantidadLibres++;
	}

	return cantidadLibres;
}

/*************************
 *
 *	Estructura de control
 *
 *************************/
int listaControl_IniciarLista(struct InformacionPagina infoBloque){
	//Inicializamos la estructura de control de codigos
	headControlCodigo = malloc(sizeof(controlCodigo_t));

	if(headControlCodigo == NULL)
		return -1;

	headControlCodigo->infoPagina = malloc(sizeof(struct InformacionPagina));

	headControlCodigo->infoPagina->pid = infoBloque.pid;
	headControlCodigo->infoPagina->pageNumber = infoBloque.pageNumber;
	headControlCodigo->infoPagina->positionInSWAP = infoBloque.positionInSWAP;
	headControlCodigo->infoPagina->bitMapPosition = infoBloque.bitMapPosition;
	headControlCodigo->next = NULL;

	return 0;
}

void listaControl_AgregarNodo(int pid, int pageNumber, int positionInSWAP, int bitMapPosition){
	if(headControlCodigo == NULL){
		struct InformacionPagina infoAAgregar;

		infoAAgregar.pid = pid;
		infoAAgregar.pageNumber = pageNumber;
		infoAAgregar.positionInSWAP = positionInSWAP;
		infoAAgregar.bitMapPosition = bitMapPosition;

		if(listaControl_IniciarLista(infoAAgregar) != 0) {
			log_error(LOG_SWAP, "La estructura de control no pudo ser creada");
			close_SwapProcess();
		}

	} else {
		struct NodoControlCodigo * current = headControlCodigo;
		while (current->next != NULL) {
			current = current->next;
		}

		current->next = malloc(sizeof(controlCodigo_t));
		current->next->infoPagina = malloc(sizeof(struct InformacionPagina));

		current->next->infoPagina->pid = pid;
		current->next->infoPagina->pageNumber = pageNumber;
		current->next->infoPagina->positionInSWAP = positionInSWAP;
		current->next->infoPagina->bitMapPosition = bitMapPosition;
		current->next->next = NULL;
	}
}

int listaControl_ObtenerPosicionEnSWAP(int pidBuscado, int pageNumber){
	struct NodoControlCodigo * current = headControlCodigo;

	for(current = headControlCodigo; current != NULL; current = current->next){
		if((current->infoPagina->pid == pidBuscado) && (current->infoPagina->pageNumber == pageNumber))
			return current->infoPagina->positionInSWAP;
	}

	return -1;
}

struct InformacionPagina listaControl_ObtenerNodoPorBitMap(int bitMap){
	struct NodoControlCodigo * current = headControlCodigo;

	struct InformacionPagina infoEncontrada;
	while (current != NULL) {
		if(current->infoPagina->bitMapPosition == bitMap){
			struct InformacionPagina info;

			info.pid = current->infoPagina->pid;
			info.pageNumber = current->infoPagina->pageNumber;
			info.positionInSWAP = current->infoPagina->positionInSWAP;
			info.bitMapPosition = current->infoPagina->bitMapPosition;

			return info;
		}

		current = current->next;
	}

	return infoEncontrada;
}

int listaControl_Longitud(){
	int length = 0;
	struct NodoControlCodigo * current;

	for(current = headControlCodigo; current != NULL; current = current->next){
		length++;
	}

	free(current);

	return length;
}

int listaControl_EliminarNodo(int pidDelProceso, int pageNumberDelProceso){
	struct NodoControlCodigo * current;
	int index = 0;

	for(current = headControlCodigo; current != NULL; current = current->next){
		if(current->infoPagina->pid == pidDelProceso && current->infoPagina->pageNumber == pageNumberDelProceso){
			listaControl_EliminarNodoIndex(index);

			return 0;
		}

		index++;
	}

	return 0;
}

int listaControl_EliminarNodoIndex(int index){
	int i = 0;
	struct NodoControlCodigo * current = headControlCodigo;
	struct NodoControlCodigo * temp_node = NULL;

	//Si es el primero, el head apunta al next
	if (index == 0) {
		temp_node = headControlCodigo->next;

		free(headControlCodigo->infoPagina);
		free(headControlCodigo);

		headControlCodigo = temp_node;

		return 0;
	}

	for(i = 0; i < index-1; i++) {
		if (current->next == NULL) {
			return -1;
		}

		current = current->next;
	}

	temp_node = current->next;
	current->next = temp_node->next;

	free(temp_node->infoPagina);
	free(temp_node);

	return 0;
}

void listaControl_EliminarLista(){
	struct NodoControlCodigo * current;
	struct NodoControlCodigo * temp_node;

	for(current = headControlCodigo; current != NULL; current = current->next){
		temp_node = current->next;

		current->next = temp_node->next;

		free(temp_node);
	}

	headControlCodigo = NULL;
}

/*************************
 *
 *		Imprimir
 *
 *************************/
void imprimir_NodosEstructuraControl(){
	puts("\n .:: NODOS ESTRUCTURA DE CONTROL ::.");

	struct NodoControlCodigo* current;
	for(current = headControlCodigo; current != NULL; current = current->next){
		printf("Nodo --> pid: %d, pageNumber: %d, positionInSWAP: %d, BitMap: %d \n", current->infoPagina->pid, current->infoPagina->pageNumber, current->infoPagina->positionInSWAP,current->infoPagina->bitMapPosition);
	}

	puts("");
}

void imprimir_EstadoBitMap(){
	int i;
	for(i = 0; i < CANTIDAD_PAGINAS ; i++){
		printf("%d",bitarray_test_bit(bitArrayStruct, i));

		if(mod(i, 100) == 0 && i!=0)
			printf("\n");

	}

	printf("\n Total paginas: %d \n", i);
}

/*************************
 *
 *		   Otras
 *
 *************************/
char obtenerPrimerChar(void* buffer){

}

int mod (int a, int b){
   int ret = a % b;

   if(ret < 0)
	   ret+=b;

   return ret;
}

void excepcionAlHablarConUMC(){
	log_error(LOG_SWAP, "An error ocurred while trying to make a send to UMC");
	close_SwapProcess();
	exit(1);
}

void dormirse(int segundos){
	log_warning(LOG_SWAP,"Sleeping for: %d seconds...", segundos);
	sleep(segundos);
}