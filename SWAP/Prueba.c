#include<stdio.h>
#include<commons/config.h>

#include<stdio.h>
#include<commons/config.h>
#include <string.h>

#define PATH_CONF "/home/utnso/configPrueba"

int main() {
	char* pathCustom = "";
    char* keys[5] = {"PUERTO_ESCUCHA","NOMBRE_SWAP","CANTIDAD_PAGINAS","TAMANIO_PAGINA","RETARDO_COMPACTACION"};

	char* PUERTO_ESCUCHA;
	char* NOMBRE_SWAP;
	char* CANTIDAD_PAGINAS;
	char* TAMANIO_PAGINA;
	char* RETARDO_COMPACTACION;

    puts(".:: Arrancando modulo SWAP ::.");
    puts("");
    puts("Si desea utilizar el archivo de configuracion por defecto presione ENTER. Si desea utilizar uno propio ingrese el path a continuacion: ");
    //scanf("%s",pathCustom);

    puts("");
    printf("%s %s", "Path ingresado: ",pathCustom);
    puts("");
    printf("%s %s", "Archivo de configuracion cargado:",PATH_CONF);

    t_config * punteroAStruct = config_create(PATH_CONF);

    if(punteroAStruct != NULL){
    	puts("");

    	int cantKeys = config_keys_amount(punteroAStruct);
    	printf("Cantidad de keys encontradas: %d \n",cantKeys);


    	int i = 0;
    	for ( i = 0; i < cantKeys; i++ ) {
    		if (config_has_property(punteroAStruct,keys[i])){
    			switch(i) {
    				case 0: PUERTO_ESCUCHA = config_get_string_value(punteroAStruct,keys[i]); break;
    				case 1: NOMBRE_SWAP = config_get_string_value(punteroAStruct,keys[i]); break;
    				case 2: CANTIDAD_PAGINAS = config_get_string_value(punteroAStruct,keys[i]); break;
    				case 3: TAMANIO_PAGINA = config_get_string_value(punteroAStruct,keys[i]); break;
    				case 4: RETARDO_COMPACTACION = config_get_string_value(punteroAStruct,keys[i]); break;
    			}

			}
    	}

    	printf("	%s --> %s \n",keys[0],PUERTO_ESCUCHA);
    	printf("	%s --> %s \n",keys[1],NOMBRE_SWAP);
    	printf("	%s --> %s \n",keys[2],CANTIDAD_PAGINAS);
    	printf("	%s --> %s \n",keys[3],TAMANIO_PAGINA);
    	printf("	%s --> %s \n",keys[4],RETARDO_COMPACTACION);


    }

    return 0;


}
