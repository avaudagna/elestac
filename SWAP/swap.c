#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include<commons/config.h>
#include<vac-commons/socketCommons/socketCommons.h>


#define PATH_CONF "swapConf"
#define PACKAGE_SIZE 1024

int main() {

	//ARCHIVO DE CONFIGURACION
	char* pathCustom = "";
    char* keys[5] = {"PUERTO_ESCUCHA","NOMBRE_SWAP","CANTIDAD_PAGINAS","TAMANIO_PAGINA","RETARDO_COMPACTACION"};

    char* IP_SWAP;
	char* PUERTO_SWAP;
	char* NOMBRE_SWAP;
	char* CANTIDAD_PAGINAS;
	char* TAMANIO_PAGINA;
	char* RETARDO_COMPACTACION;

    //CONEXION
	int swapSocket;
	int umcSocket;
	char* package;
	int readSize;
	int packageSizeRecibido;
	int bytesRecibidos = 0;
	bool done = 0;


    puts(".:: INITIALIZING SWAP ::.");
    puts("");
    puts(".::READING CONFIGURATION FILE::.");
    //puts("Si desea utilizar el archivo de configuracion por defecto presione ENTER. Si desea utilizar uno propio ingrese el path a continuacion: ");
    //scanf("%s",pathCustom);

    puts("");

    t_config * punteroAStruct = config_create(PATH_CONF);

    if(punteroAStruct != NULL){
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
    				case 2: CANTIDAD_PAGINAS = config_get_string_value(punteroAStruct,keys[i]); break;
    				case 3: TAMANIO_PAGINA = config_get_string_value(punteroAStruct,keys[i]); break;
    				case 4: RETARDO_COMPACTACION = config_get_string_value(punteroAStruct,keys[i]); break;
    			}

			}
    	}

    	printf("	%s --> %s \n",keys[0],PUERTO_SWAP);
    	printf("	%s --> %s \n",keys[1],NOMBRE_SWAP);
    	printf("	%s --> %s \n",keys[2],CANTIDAD_PAGINAS);
    	printf("	%s --> %s \n",keys[3],TAMANIO_PAGINA);
    	printf("	%s --> %s \n",keys[4],RETARDO_COMPACTACION);


    } else {
    	puts("Config file can not be loaded");
    }

    puts("");
    puts(".::INITIALIZING SERVER PROCESS::.");
    printf("%d \n",PUERTO_SWAP);

    setServerSocket(&swapSocket,IP_SWAP,PUERTO_SWAP);

    puts("");
	acceptConnection(&umcSocket, &swapSocket);

	do {
		package = (char *) malloc(sizeof(char) * PACKAGE_SIZE ) ;
		bytesRecibidos = recv(umcSocket, (void*) package, PACKAGE_SIZE, 0);
		if ( bytesRecibidos <= 0 ) {
			if ( bytesRecibidos < 0 )
				done = 1;
			}

			if (!done) {
				package[bytesRecibidos]='\0';
				printf("\nCliente: ");
				printf("%s", package);

				if(!strcmp(package,"Adios"))
					break;

				fflush(stdin);
				printf("\nServidor: ");
				fgets(package,PACKAGE_SIZE,stdin);
				package[strlen(package) - 1] = '\0';

				if ( send(umcSocket,package,strlen(package)+1,0) == -1 ) {
					perror("send");
				 	exit(1);
				}

				free(package);
			}

	  } while(!done);

	close(umcSocket);

}

