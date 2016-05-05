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


/************************
 * VARIABLES GLOBALES
 ***********************/

//CONFIG
char* IP_SWAP;
char* PUERTO_SWAP;
char* NOMBRE_SWAP;
char* PATH_SWAP;
int CANTIDAD_PAGINAS;
int TAMANIO_PAGINA;
char* RETARDO_COMPACTACION;

//CONEXION
int swapSocket;
int umcSocket;
char* package;
int readSize;
int packageSizeRecibido;
int bytesRecibidos = 0;
bool done = 0;

//SWAP
char* ABSOLUTE_PATH_SWAP;
t_bitarray* bitArrayStruct;

t_bitarray* bitArrayPrueba;

int SWAP_BLOCKSIZE;


struct pagina {
   int pid;
   int pageNumber;
   int positionInSWAP;
};

int main() {
	puts(".:: INITIALIZING SWAP ::.");
	puts("");

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

	return 0;
}

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

	do {
		package = (char *) malloc(sizeof(char) * PACKAGE_SIZE ) ;
		bytesRecibidos = recv(umcSocket, (void*) package, PACKAGE_SIZE, 0);
		if ( bytesRecibidos <= 0 ) {
			if ( bytesRecibidos < 0 )
				done = 1;
		}

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
	bitArrayStruct = bitarray_create(swapFile,SWAP_BLOCKSIZE);

	//Cerramos el puntero
	fclose(swapFile);

	int i;
	for (i = 0; i < bitarray_get_max_bit(bitArrayStruct); ++i) {
		bitarray_clean_bit(bitArrayStruct,i);
	}

	//Inicializamos la estructura de pagina
	struct pagina paginas[CANTIDAD_PAGINAS];

	for(i = 0; i < CANTIDAD_PAGINAS; i++){
		paginas[i].pid = NULL;
		paginas[i].pageNumber = NULL;
		paginas[i].positionInSWAP = i;
	}

	return 1;
}



/**********************************************************************************
 * Devuelve la posicion inicial desde donde se empiezas a escribir la o las paginas
 * Compacta si es necesario
 *
 * Devuelve -1 si el espacio disponible es insuficiente
 **********************************************************************************/
int encontrarEspacioDisponible(int cantPaginasNecesarias){
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

int escribirPagina(char* contenidoAEscribir, int posicion){
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

	//Escribimos, si es distinto al count, fallo
	if(fwrite(&contenidoObtenido,TAMANIO_PAGINA,1,fp) != 1){
		puts("Page couldn't be readed");

		return NULL;
	}

	fclose(fp);

	return contenidoObtenido;
}

void closeSwapProcess(){
	puts(".:: Terminating SWAP process ::.");

	//bitarray_destroy(bitArrayStruct);

	//fclose(swapFile);

	puts(".:: SWAP Process terminated ::.");
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
	int resultado = encontrarEspacioDisponible(11);
	printf("%d \n", resultado);


}
