#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include "socketCommons.h"


#define PATH_CONF "swapConf"
#define PACKAGE_SIZE 1024
#define IPSWAP "127.0.0.1"
#define PUERTOSWAP	45000

//ESTADOS
#define REPOSO						0
#define IDENTIFICADOR_OPERACION		1
#define HANDSHAKE					2
#define PROCESO_NUEVO				3
#define PAG_SOBRESCRITURA			4
#define PAG_PEDIDO					5
#define PROCESO_FINALIZAR			6
#define PAG_CANTIDAD				7

/************************
 * VARIABLES GLOBALES
 ***********************/

//SWAP
char* ABSOLUTE_PATH_SWAP;
t_bitarray* bitArrayStruct;
int SWAP_BLOCKSIZE;

typedef struct InformacionPagina {
   int pid;
   int pageNumber;
   int positionInSWAP;
};

typedef struct NodoControlCodigo {
    struct InformacionPagina infoPagina;
    struct NodoControlCodigo * next;
} controlCodigo_t;

struct NodoControlCodigo* headControlCodigo = NULL;

//PRUEBAS
t_bitarray* bitArrayPrueba;

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
int pedidoNuevoProceso(int pid, char* codigo);
char* pedidoLectura(int pid, int numeroPagina);
int pedidoSobrescrituraPagina(int pid, int numeroPagina, char* codigoNuevo);
int pedidoFinalizacionPrograma(int pid);

//Inicializacion
int loadConfig();
void initServer();
int createAndInitSwapFile();
int iniciarEstructuraControl();
void closeSwapProcess();

//Procesos SWAP
int encontrarEspacioDisponible(char* codigo, int cantPaginasNecesarias);
int compactar();
int marcarCodigoComoLibre(int pidDelProceso);
char* obtenerPagina(int pidDelProceso, int numeroPagina);

//Manejo de paginas
int calcularCantidadPaginasNecesarias(char* codigo);
int escribirPaginaEnSWAP(char* contenidoAEscribir, int posicion);
char* leerPagina(int posicion);
void obtenerArrayConPaginas(char* codigo, int cantPaginasNecesarias, char ** paginas);

//Estructura Control
void agregarNodoEstructuraControl(struct InformacionPagina infoPagina);
int obtenerPosicionEnSWAP(int pid, int pageNumber);
int longitudEstructuraControl();
int eliminarNodoControl(int pidDelProceso, int pageNumberDelProceso);


int main() {
	puts(".:: INITIALIZING SWAP ::.");
	puts("");

	/*
    if(!loadConfig()){
    	puts("Config file can not be loaded");
    	return -1;
    }

    puts("");

    if(!createAndInitSwapFile()){
    	puts("An error ocured while creating the swap file. Check the conf file!");
    	return -1;
    }

    //initServer();

    closeSwapProcess();

	*/

	return 0;
}

/**********************
 *
 *		PEDIDOS UMC
 *
 **********************/
int pedidoNuevoProceso(int pid, char* codigo){
	int posicionEscrituraInicial;

	//ANTES CALCULO LA CANTIDAD DE PAGINAS QUE NECESITO
	int cantPaginasNecesarias = calcularCantidadPaginasNecesarias(codigo);

	posicionEscrituraInicial = encontrarEspacioDisponible(codigo, cantPaginasNecesarias);

	if(posicionEscrituraInicial < 0){
		puts("Insufficient free space");

		return -1;
	}

	//Genero un array con las paginas a escribir
	char* paginas[cantPaginasNecesarias];
	obtenerArrayConPaginas(codigo,cantPaginasNecesarias, paginas);

	//Por cada pagina del array hago una escritura en archivo
	int i = 0;
	int cursorEscritura = posicionEscrituraInicial;
	for (i = 0; i < cantPaginasNecesarias; i++) {
		escribirPaginaEnSWAP(paginas[i], cursorEscritura);

		cursorEscritura = cursorEscritura + TAMANIO_PAGINA;
	}

	//Una vez finalizada la escritura, genero la estructura de control
	struct InformacionPagina strucControl;
	strucControl.pid = pid;
	strucControl.pageNumber = cantPaginasNecesarias;
	strucControl.positionInSWAP = posicionEscrituraInicial;


	return 1;
}

char* pedidoLectura(int pid, int numeroPagina){
	printf("[REQUEST] Page requested by UMC [PID: %d | Page number: %d] \n", pid, numeroPagina);

	char* paginaObtenida = NULL;
	paginaObtenida = obtenerPagina(pid, numeroPagina);

	if(paginaObtenida == NULL){
		puts("	An error occurred while trying to fetch the page");
		return NULL;
	}

	puts("	Page found");

	return paginaObtenida;
}

int pedidoSobrescrituraPagina(int pid, int numeroPagina, char* codigoNuevo){
	printf("[REQUEST] Page overwriting requested by UMC [PID: %d | Page number: %d] \n", pid, numeroPagina);

	int posicionPaginaEnSWAP;
	posicionPaginaEnSWAP = obtenerPosicionEnSWAP(pid, numeroPagina);

	if(posicionPaginaEnSWAP < 0){
		puts("	An error occurred while trying to fetch the page");
		return -1;
	}

	puts("	Overwriting page");
	escribirPaginaEnSWAP(codigoNuevo, posicionPaginaEnSWAP);
	puts("	The page has been overwritten");

	return 1;
}

int pedidoFinalizacionPrograma(int pid){
	printf("[REQUEST] Program finalization requested by UMC [PID: %d] \n", pid);

	int paginasLiberadas = 0;
	paginasLiberadas = marcarCodigoComoLibre(pid);

	if(paginasLiberadas > 0){
		printf("	Pages released: %d", paginasLiberadas);
		return 1;
	} else {
		puts("	Error while releasing pages");
		return -1;
	}
}


/**********************
 *
 *	INICIALIZACION
 *
 **********************/
int loadConfig(){
	//ARCHIVO DE CONFIGURACION
    char* keys[7] = {"IP_SWAP","PUERTO_ESCUCHA","NOMBRE_SWAP","PATH_SWAP","CANTIDAD_PAGINAS","TAMANIO_PAGINA","RETARDO_COMPACTACION"};

	puts(".::READING CONFIGURATION FILE::.");

	t_config * punteroAStruct = config_create(PATH_CONF);

	if(punteroAStruct != NULL) {
		printf("%s %s", "Config file loaded:",PATH_CONF);
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
					case 5: TAMANIO_PAGINA = config_get_int_value(punteroAStruct,keys[i]); break;
					case 6: RETARDO_COMPACTACION = config_get_string_value(punteroAStruct,keys[i]); break;
				}

			}
		}

		printf("	%s --> %s \n",keys[0],IP_SWAP);
		printf("	%s --> %s \n",keys[1],PUERTO_SWAP);
		printf("	%s --> %s \n",keys[2],NOMBRE_SWAP);
		printf("	%s --> %s \n",keys[3],PATH_SWAP);
		printf("	%s --> %d \n",keys[4],CANTIDAD_PAGINAS);
		printf("	%s --> %d \n",keys[5],TAMANIO_PAGINA);
		printf("	%s --> %s \n",keys[6],RETARDO_COMPACTACION);

		return 1;
	} else {
		return 0;
	}
}

void initServer(){
	puts(".::INITIALIZING SERVER PROCESS::.");

	setServerSocket(&swapSocket,IPSWAP,PUERTOSWAP);

	acceptConnection(&umcSocket, &swapSocket);

	int estado = 0;

	while(1){
		package = (char *) malloc(sizeof(char) * PACKAGE_SIZE ) ;
		bytesRecibidos = recv(umcSocket, (void*) package, PACKAGE_SIZE, 0);
		if ( bytesRecibidos <= 0 ) {
			done = 1;
		}

		/* SERIALIZACION
		 *
		 * Handshake			-->	U0|tamPag(4 bytes)				TOTAL: 7 bytes
		 * Nuevo proceso		--> U1|pid|longCodigo|codigo		TOTAL: indefinido
		 * Sobrescribir pag		--> U2|pid|nroPag|payload			TOTAL: indefinido
		 * Pedido pagina		--> U3|pid|nroPag					TOTAL:
		 * Finalizar proceso	--> U4|pid							TOTAL: 5 bytes
		 * Cant paginas disp	--> U5|cantPag
		 *
		 *
		 * TAMANIOS
		 * 		pid				--> 2 bytes
		 * 		longCodigo		--> 4 bytes
		 * 		nroPag			--> 3 bytes
		 *
		 */

		switch(estado) {
			case REPOSO:	// recibo , si lo que recibi es 'U'
				printf("\n Estoy en reposo \n");
				estado = IDENTIFICADOR_OPERACION;
				break;

			case IDENTIFICADOR_OPERACION:
				//estado = identificarOperacion(package);
				break;

			case HANDSHAKE:	// U0
				//enviarHandShakeKernel(package);
				estado = REPOSO;
				break;

			case PROCESO_NUEVO:  // U1
				estado = 1;


				break;

			default:
				printf("Error!");
				estado=REPOSO;
				break;
		}

	}

	do {

		if (!done) {
			//package[bytesRecibidos]='\0';
			printf("\nCliente: ");
			printf("%s", package);

			if(!strcmp(package,"Adios"))
				break;

			fflush(stdin);
			printf("\nServidor: ");
			//fgets(package,PACKAGE_SIZE,stdin);
			//package[strlen(package) - 1] = '\0';

			if ( send(umcSocket,(void *) "SWAP",PACKAGE_SIZE,0) == -1 ) {
				perror("send");
				exit(1);
			}

			if ( (recv(umcSocket, (void*) package, PACKAGE_SIZE, 0)) <= 0 ) {
				perror("recv");
				exit(1);

			}

			if ( send(umcSocket,(void *) package,PACKAGE_SIZE,0) == -1 ) {
							perror("send");
							exit(1);
			}

			printf("Sent :%s\n",package);

			free(package);
		}

	  } while(!done);

	close(umcSocket);
}

int createAndInitSwapFile(){
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
		puts("SWAP file can not be created");
		return 0;
	}else{
		puts("SWAP file has been successfully created");

	}

	//Obtengo el path absoluto del archivo .swap
	ABSOLUTE_PATH_SWAP = string_new();
	string_append(&ABSOLUTE_PATH_SWAP,PATH_SWAP);
	string_append(&ABSOLUTE_PATH_SWAP,NOMBRE_SWAP);

	//Creamos puntero al archivo swap
	FILE* swapFile = fopen(ABSOLUTE_PATH_SWAP,"rb");

	if (!swapFile){
		printf("Unable to open swap file! \n");
		return 0;
	}

	//Creamos y limpiamos el BitMap
	char* bitarray[CANTIDAD_PAGINAS];
	bitArrayStruct = bitarray_create(bitarray,CANTIDAD_PAGINAS/8);

	//Cerramos el puntero
	fclose(swapFile);

	int i;
	for (i = 0; i < bitarray_get_max_bit(bitArrayStruct); ++i) {
		bitarray_clean_bit(bitArrayStruct,i);
	}

	return 1;
}

int iniciarEstructuraControl(struct InformacionPagina infoBloque){
	//Inicializamos la estructura de control de codigos
	headControlCodigo = malloc(sizeof(controlCodigo_t));

	if (headControlCodigo == NULL)
		return 1;

	headControlCodigo->infoPagina = infoBloque;
	headControlCodigo->next = NULL;

	return 0;
}

void closeSwapProcess(){
	puts(".:: Terminating SWAP process ::.");

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
int encontrarEspacioDisponible(char* codigo, int cantPaginasNecesarias){
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
		return compactar(); //Hay lugar pero hay que compactar

	return -1; //No hay lugar :(
}

int compactar(){
	return -2;
}

int marcarCodigoComoLibre(int pidDelProceso){
	int modificaciones = 0;
	controlCodigo_t * current = headControlCodigo;
	controlCodigo_t * temp_node;

	for(current = headControlCodigo; current != NULL; current = current->next){
		if(current->infoPagina.pid == pidDelProceso){
			bitarray_clean_bit(bitArrayStruct,current->infoPagina.positionInSWAP); //Lo pongo como disponible

			//Elimino el nodo de la estructura de control
			temp_node = current->next;
			current->next = temp_node->next;
			free(temp_node);

			modificaciones++;
		}
	}

	return modificaciones;
}

char* obtenerPagina(int pidDelProceso, int numeroPagina){
	int posicionEnSWAP = obtenerPosicionEnSWAP(pidDelProceso,numeroPagina);

	if(posicionEnSWAP > -1){
		return leerPagina(posicionEnSWAP);
	}

	return NULL;
}

/**********************
 *
 *	Manejo de paginas
 *
 **********************/

int calcularCantidadPaginasNecesarias(char* codigo){
	int tamanioCodigo =  string_length(codigo);

	if(tamanioCodigo%TAMANIO_PAGINA){
		return (tamanioCodigo/TAMANIO_PAGINA)+1;
	} else {
		return tamanioCodigo/TAMANIO_PAGINA;
	}

}

int escribirPaginaEnSWAP(char* contenidoAEscribir, int posicion){
	FILE* fp = fopen(ABSOLUTE_PATH_SWAP,"rb");

	if(fp == NULL){
		puts("Page couldn't be written");

		return -1;
	}

	fseek(fp,posicion,SEEK_SET);

	//Escribimos, si es distinto al count, fallo
	if(fwrite(contenidoAEscribir,TAMANIO_PAGINA,1,fp) != 1){
		puts("Page couldn't be written");

		return -1;
	}

	fclose(fp);

	return 0;
}

char* leerPagina(int posicion){
	FILE* fp = fopen(ABSOLUTE_PATH_SWAP,"rb");

	if(fp == NULL){
		puts("Page couldn't be readed");

		return NULL;
	}

	fseek(fp,posicion,SEEK_SET);

	char* contenidoObtenido;

	//Leemos, si es distinto al count, fallo
	if(fread(contenidoObtenido,TAMANIO_PAGINA,1,fp) != 1){
		puts("Page couldn't be readed");

		return NULL;
	}

	fclose(fp);

	return contenidoObtenido;
}

//Cargo en un array el codigo dividido en paginas
void obtenerArrayConPaginas(char* codigo, int cantPaginasNecesarias, char ** paginas){
	char* buffer = *paginas;

	int i, cursor = 0;
	for (i = 0; i < cantPaginasNecesarias; i++) {
		char* codigoAEscribir = string_substring(codigo,cursor,TAMANIO_PAGINA);

		buffer[i] = codigoAEscribir;

		if(i > 0)
			cursor = TAMANIO_PAGINA * i;

	}
}

/*************************
 *
 *	Estructura de control
 *
 *************************/
void agregarNodoEstructuraControl(struct InformacionPagina infoPagina){
	if(headControlCodigo == NULL){
		if(iniciarEstructuraControl(infoPagina) == 1)
			puts("La estructura no pudo ser creada");

	} else {
		controlCodigo_t * current = headControlCodigo;
		while (current->next != NULL) {
			current = current->next;
		}

		/* now we can add a new variable */
		current->next = malloc(sizeof(controlCodigo_t));
		current->infoPagina = infoPagina;
		current->next->next = NULL;
	}

}

int obtenerPosicionEnSWAP(int pidBuscado, int pageNumber){
	controlCodigo_t * current = headControlCodigo;

	while (current != NULL) {
		if(current->infoPagina.pid == pidBuscado && current->infoPagina.pageNumber == pageNumber)
			return current->infoPagina.positionInSWAP;

		current = current->next;
	}

	return -1;
}

int longitudEstructuraControl(){
	int length = 0;
	struct NodoControlCodigo * current;

	for(current = headControlCodigo; current != NULL; current = current->next){
		length++;
	}

	return length;
}

int eliminarNodoControl(int pidDelProceso, int pageNumberDelProceso){
	struct NodoControlCodigo * current;
	struct NodoControlCodigo * temp_node = NULL;

	if(pageNumberDelProceso < 0){ //Eliminamos todos los nodos relacionados con el pid requerido
		for(current = headControlCodigo; current != NULL; current = current->next){
			if(current->infoPagina.pid == pidDelProceso){
				temp_node = current->next;

				current->next = temp_node->next;

				free(temp_node);
			}
		}

	} else { //Eliminamos solo el que matchea con el pid y el pageNumber
		for(current = headControlCodigo; current != NULL; current = current->next){
				if(current->infoPagina.pid == pidDelProceso && current->infoPagina.pageNumber == pageNumberDelProceso){
					temp_node = current->next;

					current->next = temp_node->next;

					free(temp_node);

					return 0;
				}
		}
	}

	return 0;
}

/****************************
 *
 * 			PRUEBAS
 *
 ****************************/
void pruebaFuncEncontrar(){
	puts(".:: Comienza prueba ::.");
	puts("");

	FILE* swapFile = fopen(ABSOLUTE_PATH_SWAP,"rb");

	//Creamos y limpiamos el BitMap
	bitArrayPrueba = bitarray_create(swapFile,10);

	int i;
	for (i = 0; i < bitarray_get_max_bit(bitArrayPrueba); ++i) {
		bitarray_clean_bit(bitArrayPrueba,i);
	}

	puts("BitMap creado e inicializado");
	puts("");

	puts("Estructura: 1 0 0 1 0 0 1 1 1 1");
	bitarray_set_bit(bitArrayPrueba,0);
	//bitarray_set_bit(bitArrayPrueba,1);
	bitarray_set_bit(bitArrayPrueba,2);
	//bitarray_set_bit(bitArrayPrueba,3);
	bitarray_set_bit(bitArrayPrueba,6);
	bitarray_set_bit(bitArrayPrueba,7);
	bitarray_set_bit(bitArrayPrueba,8);
	bitarray_set_bit(bitArrayPrueba,9);
	for (i = 0; i < 10; ++i) {
		printf("%d",bitarray_test_bit(bitArrayPrueba,i));
	}

	puts("");
	puts("Queremos encontrar un espacio consecutivo de 3 paginas");
	//int resultado = encontrarEspacioDisponible(11);
	//printf("%d \n", resultado);


}

void pruebaLista(){
	struct InformacionPagina info1;
	struct InformacionPagina info2;
	struct InformacionPagina info3;
	struct InformacionPagina info4;

	info1.pid = 1;
	info1.pageNumber = 5432;
	info1.positionInSWAP = 89;

	info2.pid = 2;
	info2.pageNumber = 21312;
	info2.positionInSWAP = 421;

	info3.pid = 3;
	info3.pageNumber = 47821;
	info3.positionInSWAP = 654;

	info4.pid = 2;
	info4.pageNumber = 574;
	info4.positionInSWAP = 421;

	agregarNodoEstructuraControl(info1);
	agregarNodoEstructuraControl(info2);
	agregarNodoEstructuraControl(info3);
	agregarNodoEstructuraControl(info4);

	printf("%d \n", obtenerPosicionEnSWAP(2,21312));
	printf("size: %d \n",longitudEstructuraControl());
	printf("%d \n", eliminarNodoControl(2,-1));
	printf("size: %d \n",longitudEstructuraControl());

}
