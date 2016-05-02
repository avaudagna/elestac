#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <commons/config.h>
#include <commons/string.h>
#include "socketCommons.h"


#define PATH_CONF "swapConf"
#define PACKAGE_SIZE 1024
#define IPSWAP "127.0.0.1"
#define PUERTOSWAP	45000

//CONFIG
char* IP_SWAP;
char* PUERTO_SWAP;
char* NOMBRE_SWAP;
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

int blockSizeSWAP;

int main() {
	puts(".:: INITIALIZING SWAP ::.");
	puts("");

    loadConfig();
    puts("");

    //initServer();

    createSwapFile();

	return 0;
}

void loadConfig(){
	//ARCHIVO DE CONFIGURACION
    char* keys[5] = {"PUERTO_ESCUCHA","NOMBRE_SWAP","CANTIDAD_PAGINAS","TAMANIO_PAGINA","RETARDO_COMPACTACION"};

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
					case 0: PUERTO_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
					case 1: NOMBRE_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
					case 2: CANTIDAD_PAGINAS = config_get_int_value(punteroAStruct,keys[i]); break;
					case 3: TAMANIO_PAGINA = config_get_int_value(punteroAStruct,keys[i]); break;
					case 4: RETARDO_COMPACTACION = config_get_string_value(punteroAStruct,keys[i]); break;
				}

			}
		}

		printf("	%s --> %s \n",keys[0],PUERTO_SWAP);
		printf("	%s --> %s \n",keys[1],NOMBRE_SWAP);
		printf("	%s --> %d \n",keys[2],CANTIDAD_PAGINAS);
		printf("	%s --> %d \n",keys[3],TAMANIO_PAGINA);
		printf("	%s --> %s \n",keys[4],RETARDO_COMPACTACION);


	} else {
		puts("Config file can not be loaded");
	}
}

void initServer(){
	puts(".::INITIALIZING SERVER PROCESS::.");
	puts("");

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

void createSwapFile(){
	puts(".: CREATING SWAP FILE :.");

	char* comandoCrearSwap = string_new();

	//dd syntax ---> dd if=/dev/zero of=foobar count=1024 bs=1024;
	string_append(&comandoCrearSwap, "dd");

	//Utilizo el archivo zero que nos va a dar tantos NULL como necesite
	string_append(&comandoCrearSwap, " if=/dev/zero");

	//El archivo de destino es mi swap a crear
	string_append(&comandoCrearSwap, " of=/home/utnso/");
	if(NOMBRE_SWAP != NULL)
		string_append(&comandoCrearSwap, NOMBRE_SWAP);
	else
		string_append(&comandoCrearSwap, "pruebaSWAP");

	//Especifico el BLOCKSIZE, que seria el total de mi swap
	string_append(&comandoCrearSwap, " bs=");
	string_append(&comandoCrearSwap, string_itoa(TAMANIO_PAGINA*CANTIDAD_PAGINAS)); //Tamanio total: tam_pag * cant_pag

	//Con count estipulo la cantidad de bloques
	string_append(&comandoCrearSwap, " count=");
	string_append(&comandoCrearSwap, "1");

	printf("%s %s \n", "Command executed --->",comandoCrearSwap);

	if( system(comandoCrearSwap) )
		puts("SWAP file can not be created");
	else
		puts("SWAP file created");

}
