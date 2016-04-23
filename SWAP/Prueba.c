#include<stdio.h>
#include<commons/config.h>

int main() {
    printf("Hola hola");

    char * conf = "/home/utnso/configPrueba";

    //int *p = malloc(sizeof(conf) + 1);

    t_config * punteroAStruct = config_create(conf);

    // printf(*punteroAStruct->*path);


}
