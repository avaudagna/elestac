#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

//Librerias propias
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include "socketCommons.h"


//OPERACIONES UMC
#define ESCRIBIR 1
#define LECTURA 2
#define FINALIZARPROG 3

#define PATH_CONF "/home/alan/repos/tp-2016-1c-Vamo-a-calmarno/SWAP/swapConf"
#define PACKAGE_SIZE 1024
#define IPSWAP "127.0.0.1"
//#define IPSWAP "192.168.0.28"
#define PUERTOSWAP 6800


/************************
 * VARIABLES GLOBALES
 ***********************/

//SWAP
char* ABSOLUTE_PATH_SWAP;
char* bitMap;
t_bitarray* bitArrayStruct;
int SWAP_BLOCKSIZE;

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
int readSize;
int packageSizeRecibido;
int bytesRecibidos = 0;
bool done = 0;

//CONFIG
char* IP_SWAP;
char* PUERTO_SWAP;
char* NOMBRE_SWAP;
char* PATH_SWAP;
int CANTIDAD_PAGINAS;
int TAMANIO_PAGINA;
char* RETARDO_COMPACTACION;

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

int main() {
	puts(".:: INITIALIZING SWAP ::. \n");

	if(!init_Config()){
    	puts("Config file can not be loaded");
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

	void* buffer = malloc(tamanioTrama);

	if(recv(umcSocket, buffer,tamanioTrama,0)<= 0){
		free(buffer);
		return handshakeOK;
	}

	//Obtenemos el primer caracter
	memcpy(&handShake, buffer,sizeof(char));

	if(handShake == 'U'){ //El handshake es correcto
		char aux[20], aux2[20];

		memcpy(&aux, buffer, tamanioTrama);
		int i, j=0;
		for(i = 0; i < 20;i++){
			if( aux[i] >= '0' && aux[i] <= '9' ){
				aux2[j] = aux[i];
				j++;
			}
		}

		TAMANIO_PAGINA = atoi(aux2);

		init_BitMap();

		//Armo la trama a responder char[]: S+cantPaginasLibres
		int SIZE_TRAMA = 5;
		char* respuesta = malloc(SIZE_TRAMA);
		sprintf(respuesta,"S%04d", paginas_CantidadPaginasLibres());

		if(send(umcSocket,(void *) respuesta, SIZE_TRAMA,0) == -1){
			puts("[ERROR] Error while trying to complete handshake");
			return handshakeOK;
		}

        free(respuesta);

        printf("[INFO] Handshake with UMC succesfully done. Page size received: %d \n", TAMANIO_PAGINA);

		handshakeOK = 1;

		if(!init_SwapFile()){
			puts("[ERROR] SWAP file couldn't be created. Check the conf file!");
			close_SwapProcess();
		}
	} else {
		char tramaRecibida[100];
		memcpy(&tramaRecibida, buffer, tamanioTrama);

		printf("[ERROR] Error while trying to complete handshake. Code recieved: %s \n", tramaRecibida);
	}

	free(buffer);
	return handshakeOK;
}

char* request_Lectura(int pid, int numeroPagina){
	printf("[REQUEST] Page requested by UMC [PID: %d | Page number: %d] \n", pid, numeroPagina);

	char* paginaObtenida = NULL;
	paginaObtenida = swap_ObtenerPagina(pid, numeroPagina);

	if(paginaObtenida == NULL){
		puts("--> An error occurred while trying to fetch the page");
		return NULL;
	}

	puts("--> Page found!");

	return paginaObtenida;
}

int request_EscrituraPagina(int pid, int numeroPagina, char* codigo){
	printf("[REQUEST] Page writing requested by UMC [PID: %d | Page number: %d] \n", pid, numeroPagina);

	char* paginaObtenida = NULL;
	paginaObtenida = swap_ObtenerPagina(pid, numeroPagina);

	if(paginaObtenida != NULL){ //Si la pagina ya existe, la sobreescribo
		puts("--> Requested page alreay exists. Overwriting existing page");

		int posicionPaginaEnSWAP;
		posicionPaginaEnSWAP = listaControl_ObtenerPosicionEnSWAP(pid, numeroPagina);

		if(posicionPaginaEnSWAP < 0){
			puts("--> An error occurred while trying to fetch the page");
			return -1;
		}

		paginas_EscribirPaginaEnSWAP(codigo, posicionPaginaEnSWAP);
		puts("--> The page has been succesfully overwritten! \n");

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

	puts("--> The page has been written!");

	return 0;
}

int request_FinalizacionPrograma(int pid){
	printf("[REQUEST] Program finalization requested by UMC [PID: %d] \n", pid);

	int paginasLiberadas = 0;
	paginasLiberadas = swap_LiberarPrograma(pid);

	if(paginasLiberadas > 0){
		printf("--> Pages released: %d \n", paginasLiberadas);
		return 1;
	} else {
		puts("--> Error while releasing pages");
		return -1;
	}

}

/**********************
 *
 *	INICIALIZACION
 *
 **********************/
int init_Config(){
	//ARCHIVO DE CONFIGURACION
    char* keys[7] = {"IP_SWAP","PUERTO_ESCUCHA","NOMBRE_SWAP","PATH_SWAP","CANTIDAD_PAGINAS","TAMANIO_PAGINA","RETARDO_COMPACTACION"};

	puts(".:: READING CONFIGURATION FILE ::. \n");

	t_config * punteroAStruct = config_create(PATH_CONF);

	if(punteroAStruct != NULL) {
		printf("%s %s", "[INFO] Config file loaded:",PATH_CONF);
		puts("");

		int cantKeys = config_keys_amount(punteroAStruct);
		printf("Number of keys found: %d \n",cantKeys);

		int i = 0;
		for ( i = 0; i < cantKeys; i++ ) {
			if (config_has_property(punteroAStruct,keys[i])){
				switch(i) {
					case 0: IP_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
					case 1: PUERTO_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
					case 2: NOMBRE_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
					case 3: PATH_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
					case 4: CANTIDAD_PAGINAS = config_get_int_value(punteroAStruct,keys[i]); break;
					case 5: RETARDO_COMPACTACION = config_get_string_value(punteroAStruct,keys[i]); break;
				}

			}
		}

		printf("	%s --> %s \n",keys[0],IP_SWAP);
		printf("	%s --> %s \n",keys[1],PUERTO_SWAP);
		printf("	%s --> %s \n",keys[2],NOMBRE_SWAP);
		printf("	%s --> %s \n",keys[3],PATH_SWAP);
		printf("	%s --> %d \n",keys[4],CANTIDAD_PAGINAS);
		printf("	%s --> %s \n",keys[5],RETARDO_COMPACTACION);

		free(punteroAStruct);

		return 1;
	} else {
		free(punteroAStruct);

		return 0;
	}
}

void init_Server(){
	puts(".:: INITIALIZING SERVER PROCESS ::. \n");

	setServerSocket(&swapSocket,IP_SWAP,atoi(PUERTO_SWAP));

	acceptConnection(&umcSocket, &swapSocket);

	void* buffer = malloc(sizeof(int));
	int handshakeOK = 0;
	int operacion = 0;

	while(1){
		if(handshakeOK){
			buffer = malloc(sizeof(int));

			puts("[INFO] Waiting for UMC request");

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
					printf("[Error] Request code unknown: %d \n", operacion);

					break;
			}

			operacion = -1;

		} else { //HAGO EL HANDSHAKE
			handshakeOK = umc_handshake();

			char error[1] = {'E'};
			if(send(umcSocket,(void *) error, 1,0) == -1) {
				perror("send");
			}

		}

	}

}

void umc_leer(){
	int pid;
	int numeroPagina;

	char* buffer = malloc(sizeof(int));

	//Recibo el PID
	if(recv(umcSocket, buffer, 1, 0)<= 0)
		exit(1);

	//Lo convertimos a int
	char aux;
	memcpy(&aux,buffer,1);
	pid = aux - '0';

	free(buffer);
	buffer = malloc(sizeof(int));

	//Recibo el numero pagina
	if(recv(umcSocket, buffer, 1, 0)<= 0)
		exit(1);

	//Lo convertimos a int
	memcpy(&aux,buffer,1);
	numeroPagina = aux - '0';

	free(buffer);
	buffer = malloc(sizeof(char[TAMANIO_PAGINA]));

	//Hacemos el request
	char* resultadoRequest = request_Lectura(pid, numeroPagina);

	if(resultadoRequest != NULL){ //ENVIAMOS A UMC 1+cantPaginasLibres
		char* respuesta = malloc(sizeof(char[TAMANIO_PAGINA]));
		sprintf(respuesta,"%s", resultadoRequest);

		if(send(umcSocket,(void *) respuesta, TAMANIO_PAGINA,0) == -1){
			perror("send");
			exit(1);
		}

		free(respuesta);

	} else { //ENVIAMOS ERROR A UMC
		char error[1] = {'E'};
		if(send(umcSocket,(void *) error, 1,0) == -1) {
			perror("send");
		}
	}

	free(buffer);
}

void umc_escribir(){
	int SIZE_TRAMA = 5;

	char* buffer = malloc(sizeof(int));

	int pid;
	int numeroPagina;
	char* codigo = malloc(sizeof(char[TAMANIO_PAGINA]));

	//Recibo el PID
	if(recv(umcSocket, buffer, sizeof(int), 0)<= 0) {
		exit(1); }

	pid = atoi(buffer);

	free(buffer);
	buffer = malloc(sizeof(int));

	//Recibo el numero pagina
	if(recv(umcSocket, buffer, sizeof(int), 0)<= 0) {
		exit(1); }
	numeroPagina = atoi(buffer);

	free(buffer);
	buffer = malloc(sizeof(char[TAMANIO_PAGINA]));

	//Recibimos el codigo
	if(recv(umcSocket, buffer, sizeof(char)*TAMANIO_PAGINA, 0)<= 0) {
		exit(1); }

	memcpy(codigo, buffer, sizeof(char)*TAMANIO_PAGINA);

	//Hacemos el request
	int resultadoRequest = request_EscrituraPagina(pid, numeroPagina, codigo);

	if(resultadoRequest > -1){ //ENVIAMOS A UMC 1+cantPaginasLibres
		char* respuesta;

		respuesta = malloc(sizeof(char)*SIZE_TRAMA);
		sprintf(respuesta,"1%04d", paginas_CantidadPaginasLibres());

		if(send(umcSocket,(void *) respuesta, SIZE_TRAMA,0) == -1){
			perror("send");
			exit(1);
		}

		free(respuesta);

	} else { //ENVIAMOS ERROR A UMC
		char error[1] = {'E'};
		if(send(umcSocket,(void *) error, 1,0) == -1) {
			perror("send");
		}
	}

	free(buffer);
	free(codigo);

}

void umc_finalizarPrograma(){
	int SIZE_TRAMA = 5;

	char* buffer = malloc(sizeof(int));

	int pid;


	//Recibo el PID
	if(recv(umcSocket, buffer, 1, 0)<= 0)
		exit(1);

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
		sprintf(respuesta,"S%d", paginas_CantidadPaginasLibres());

		if(send(umcSocket,(void *) respuesta, SIZE_TRAMA,0) == -1){
			perror("send");
			exit(1);
		}

		free(respuesta);

	} else { //ENVIAMOS ERROR A UMC
		char error[1] = {'E'};
		if(send(umcSocket,(void *) error, 1,0) == -1) {
			perror("send");
			exit(1);
		}
	}

	free(buffer);
}

int init_SwapFile(){
	puts(".: CREATING SWAP FILE :.");

	char* comandoCrearSwap = string_new();

	//dd syntax ---> dd if=/dev/zero of=foobar count=1024 bs=1024;
	string_append(&comandoCrearSwap, "dd");

	//Utilizo el archivo zero que nos va a dar tantos NULL como necesite
	string_append(&comandoCrearSwap, " if=/dev/zero");

	//El archivo de destino es mi swap a crear
	string_append(&comandoCrearSwap, " of=");
	if(PATH_SWAP != NULL)
		string_append(&comandoCrearSwap, PATH_SWAP);
	else
		string_append(&comandoCrearSwap, "/home/utnso/");

	if(NOMBRE_SWAP != NULL)
		string_append(&comandoCrearSwap, NOMBRE_SWAP);
	else
		string_append(&comandoCrearSwap, "pruebaSWAP");

	//Especifico el BLOCKSIZE, que seria el total de mi swap
	SWAP_BLOCKSIZE = TAMANIO_PAGINA*CANTIDAD_PAGINAS;
	string_append(&comandoCrearSwap, " bs=");
	string_append(&comandoCrearSwap, string_itoa(SWAP_BLOCKSIZE)); //Tamanio total: tam_pag * cant_pag

	//Con count estipulo la cantidad de bloques
	string_append(&comandoCrearSwap, " count=");
	string_append(&comandoCrearSwap, "1");

	//printf("%s %s \n", "Command executed --->",comandoCrearSwap);

	if( system(comandoCrearSwap) ){
		puts("--> SWAP file can not be created");
		free(comandoCrearSwap);
		return 0;
	}else{
		puts("[INFO] SWAP file has been successfully created!");
		free(comandoCrearSwap);
	}

	//Obtengo el path absoluto del archivo .swap
	ABSOLUTE_PATH_SWAP = string_new();
	string_append(&ABSOLUTE_PATH_SWAP,PATH_SWAP);
	string_append(&ABSOLUTE_PATH_SWAP,NOMBRE_SWAP);

	//Creamos puntero al archivo swap
	FILE* swapFile = fopen(ABSOLUTE_PATH_SWAP,"rb");

	if (!swapFile){
		printf("--> Unable to open swap file! \n");
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
	puts(".:: Terminating SWAP process ::.");

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
	free(NOMBRE_SWAP);
	free(PATH_SWAP);
	free(RETARDO_COMPACTACION);

	//Eliminamos la lista con todos sus modulos
	listaControl_EliminarLista();
	free(headControlCodigo);

	//Liberamos el BitMap
	bitarray_destroy(bitArrayStruct);

	puts(".:: SWAP Process terminated ::.");
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

int swap_Compactar(){
	puts(".:: DEFRAGMETATION REQUESTED ::. \n");

	puts(".:: SWAP STATE BEFORE DEFRAGMETATION ::. \n");
	imprimir_EstadoBitMap();

	puts("\n .:: BEGINING DEFRAGMETATION ::. \n");

	sleep(*RETARDO_COMPACTACION);

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
	FILE* fp = fopen(ABSOLUTE_PATH_SWAP,"r+b");

	char pagina[TAMANIO_PAGINA];
	int i;
	for(i = 0; i < TAMANIO_PAGINA; i++){
		pagina[i] = contenidoAEscribir[i];
	}

	if(!fp){
		puts("--> Page couldn't be written");
		fclose(fp);
		free(contenidoAEscribir);

		return -1;
	}

	if(fseek(fp,posicion,SEEK_SET)){
		puts("--> Invalid pointer");
		fclose(fp);
		free(contenidoAEscribir);

		return -1;
	}

	int escritoEnPosicion = ftell(fp);

	//Escribimos, si es distinto al count, fallo
	if(fwrite(pagina,TAMANIO_PAGINA,1,fp) != 1){
		puts("--> Page couldn't be written");
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
		puts("--> File couldn't be opened");
		return NULL;
	}

	fseek(fp,posicion,SEEK_SET);

	char* contenidoObtenido = malloc(TAMANIO_PAGINA*sizeof(char)); //CUIDADO ACA

	//Leemos, si es distinto al count, fallo

	if(fread(contenidoObtenido,TAMANIO_PAGINA,1,fp) != 1){
		puts("--> Page couldn't be readed");

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
		return 1;

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

		if(listaControl_IniciarLista(infoAAgregar) == 1)
			puts("La estructura no pudo ser creada");

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

	printf("\n Total: %d \n", i);
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
