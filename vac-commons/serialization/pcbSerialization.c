
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

void serialize_puntero_char(char* etiquetas, char** buffer);
void serialize_t_intructions(t_intructions* instrucciones_serializado, char** buffer);
void serlize_t_size(t_size instrucciones_size, char** buffer);
void serialize_int(int num, char** buffer);
int cant_digitos (int n);

void serialize_t_metadata_program(t_metadata_program *x, char **output) {
    *output = (char*) calloc (1, sizeof(t_metadata_program));
    serialize_t_puntero_instruccion(x->instruccion_inicio, output);
	serlize_t_size(x->instrucciones_size, output);
	serialize_t_intructions(x->instrucciones_serializado, output);
	serlize_t_size(x->etiquetas_size, output);
	serialize_puntero_char(x->etiquetas, output);
	serialize_int(x->cantidad_de_funciones, output);
	serialize_int(x->cantidad_de_etiquetas, output);
}



void serialize_t_puntero_instruccion(t_puntero_instruccion posicion, char** buffer) {
	int digitos = cant_digitos(posicion);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", posicion);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}


void serialize_puntero_char(char* etiquetas, char** buffer) {
	int digitos = cant_digitos(etiquetas);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", etiquetas);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}

void serialize_t_intructions(t_intructions* instrucciones_serializado, char** buffer) {
	int digitos = cant_digitos(instrucciones_serializado->start) + cant_digitos((int)instrucciones_serializado->offset);
	char* str = malloc(digitos+1);
	serialize_t_puntero_instruccion(instrucciones_serializado->start, buffer );
	serlize_t_size(instrucciones_serializado->offset, buffer);
	strcat(*buffer,str);
	free(str);
}

void serlize_t_size(t_size instrucciones_size, char** buffer) {
	int digitos = cant_digitos((int) instrucciones_size);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", instrucciones_size);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}

void serialize_int(int num, char** buffer) {
	int digitos = cant_digitos(num);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", num);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}

int cant_digitos (int n) {
	int i = 0;

	if (n==0) {
		return 1;
	}
	else if (n>0) {
		for (i=1; n/10 > 0 ; i++) {
			n = n/10;
		}
		return i;
	}
	else {
		return 0;
	}
}

int main (void) {
	char *programita = "begin\nvariables f, A, g \n A = 	0  \n!Global = 1+A\n  print !Global \n jnz !Global Siguiente \n:Proximo\n	 \n f = 8	   \ng <- doble !Global	\n  io impresora 20\n	:Siguiente	 \n imprimir A  \ntextPrint  Hola Mundo!  \n\n  sumar1 &g	 \n print g  \n\n  sinParam \n \nend\n\nfunction sinParam\n	textPrint Bye\nend\n\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f  \n f = $0 + $0 \n  return fvend\n\nfunction sumar1\n	*$0 = 1 + *$0\nend\n\nfunction imprimir\n  wait mutexA\n    print $0+1\n  signal mutexB\nend\n\n";
	t_metadata_program *estructura = metadata_desde_literal(programita);
	char* buffer = (char*) calloc(1,SIZEOF_PCB) ;

	serialize_t_metadata_program(estructura, &buffer);
	puts(buffer);
	return 0;
}
