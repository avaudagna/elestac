
#include <stdio.h>
#include <stdlib.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <commons/string.h>
#include <string.h>

typedef u_int32_t t_puntero;
typedef u_int32_t t_size;
typedef u_int32_t t_puntero_instruccion;

#define SIZEOF_PCB 32

void deserialize_t_metadata_program(t_metadata_program **x, char *serialized_data);

void deserialize_t_metadata_program(t_metadata_program **x, char *serialized_data) {
	t_metadata_program * estructura = *x;
	const char separator[2] = "-";
	estructura->instruccion_inicio = (t_puntero_instruccion) atoi(strtok(serialized_data, separator));
    estructura->instrucciones_size = (t_size) atoi(strtok(NULL, separator));
    estructura->instrucciones_serializado->start = (t_puntero_instruccion) atoi(strtok(NULL, separator));
    estructura->instrucciones_serializado->offset = (t_size) atoi(strtok(NULL, separator));
    estructura->etiquetas_size = (t_size) atoi(strtok(NULL, separator));
    estructura->etiquetas = strtok(NULL, separator);
    estructura->cantidad_de_funciones = atoi(strtok(NULL, separator));
    estructura->cantidad_de_etiquetas = atoi(strtok(NULL, separator));
}

int main (void) {
	char *programita = "0-25-6-19-73-159953784-4-2-";
	t_metadata_program* estructura = (t_metadata_program*) calloc(1,sizeof(t_metadata_program));
	deserialize_t_metadata_program(&estructura, programita);
	puts("Debug estructura");
	return 0;
}


