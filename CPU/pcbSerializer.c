
#include <stdio.h>
#include <stdlib.h>

typedef u_int32_t t_puntero;
typedef u_int32_t t_size;
typedef u_int32_t t_puntero_instruccion;


typedef struct {
	t_puntero_instruccion	start;
	t_size		offset;
} t_intructions;

typedef struct {
	t_puntero_instruccion	instruccion_inicio;	//El numero de la primera instruccion (Begin)
	t_size			instrucciones_size;				// Cantidad de instrucciones
	t_intructions*	instrucciones_serializado; 		// Instrucciones del programa

	t_size			etiquetas_size;					// Tamaño del mapa serializado de etiquetas
	char*			etiquetas;							// La serializacion de las etiquetas

	int				cantidad_de_funciones;
	int				cantidad_de_etiquetas;
} t_metadata_program;


void serialize_puntero_char(char* etiquetas, char** buffer);
void serialize_t_intructions(t_intructions* instrucciones_serializado, char** buffer);
void serlize_t_size(t_size instrucciones_size, char** buffer);
void serialize_int(int num, char** buffer);

void serialize_t_metadata_program(t_metadata_program *x, char **output) {
    *output = (char*) calloc (1, sizeof(t_metadata_program));
	serialize_int(x->instruccion_inicio, output);
	serlize_t_size(x->instrucciones_size, output);
	serialize_t_intructions(x->instrucciones_serializado, output);
	serlize_t_size(x->etiquetas_size, output);
	serialize_puntero_char(x->etiquetas, output);
	serialize_int(x->cantidad_de_funciones, output);
	serialize_int(x->cantidad_de_etiquetas, output);
}

void serialize_puntero_char(char* etiquetas, char** buffer) {
	//etiquetas = htonl(etiquetas);
	memcpy((char *)  ((*buffer)+strlen(*buffer)), &etiquetas, sizeof(char*));
	//strcat(*buffer, etiquetas);
}

void serialize_t_intructions(t_intructions* instrucciones_serializado, char** buffer) {
	//instrucciones_serializado = htonl(instrucciones_serializado);
	memcpy((char *)  ((*buffer)+strlen(*buffer)), &instrucciones_serializado, sizeof(t_intructions));
	//strcat(*buffer, instrucciones_serializado);
}

void serlize_t_size(t_size instrucciones_size, char** buffer) {
	//instrucciones_size = htonl(instrucciones_size);
	memcpy((char *)  ((*buffer)+strlen(*buffer)), &instrucciones_size, sizeof(t_size));
	//strcat(*buffer, instrucciones_size);
}

void serialize_int(int num, char** buffer) {
	//num = htonl(num);
	memcpy((char *) ((*buffer)+strlen(*buffer)), &num, sizeof(int));
	//strcat(*buffer, num);
}

//int main (void) {
//	t_metadata_program *estructura = (t_metadata_program*) calloc(1,sizeof(t_metadata_program));
//	char* buffer = (char*) calloc(1,sizeof(t_metadata_program)) ;
//	estructura->instruccion_inicio = (t_puntero_instruccion) 1;
//	estructura->instrucciones_size = (t_size) 2;
//	estructura->instrucciones_serializado = (t_intructions*) 3;
//	estructura->etiquetas_size = (t_size) 4;
//	estructura->etiquetas = (char*) 5;
//	estructura->cantidad_de_funciones = (int) 6;
//	estructura->cantidad_de_etiquetas = (int) 7;
//
//	serialize_t_metadata_program(estructura, &buffer);
//	printf("%s\n", buffer);
//	return 0;
//}

