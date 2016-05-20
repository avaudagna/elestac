
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <commons/string.h>
//#include <string.h>

typedef u_int32_t t_puntero;
typedef u_int32_t t_size;
typedef u_int32_t t_puntero_instruccion;

#define SIZEOF_PCB 32

void serialize_t_metadata_program(t_metadata_program *x, char **output);
void serialize_int(int num, char** buffer);
void serialize_t_intructions(int cantidad_instrucciones,t_intructions* instrucciones_serializado, char** buffer);
//void serialize_t_puntero_instruccion(t_puntero_instruccion posicion, char** buffer);
//void serialize_puntero_char(char* etiquetas, char** buffer);
//void serialize_t_size(t_size instrucciones_size, char** buffer);


int cant_digitos (int n);

void serialize_t_metadata_program(t_metadata_program *x, char **output) {
    *output = (char*) calloc (1, sizeof(t_metadata_program));
    serialize_int(x->instruccion_inicio, output);			// serializar program counter
    serialize_int(x->instrucciones_size, output);			// serializar cantidad de instrucciones
	serialize_t_intructions(x->instrucciones_size,x->instrucciones_serializado, output);	// serializar indice de codigo
	//serialize_t_size(x->etiquetas_size, output);
	//serialize_puntero_char(x->etiquetas, output);
	//serialize_int(x->cantidad_de_funciones, output);
	//serialize_int(x->cantidad_de_etiquetas, output);
}

/*
void serialize_t_puntero_instruccion(t_puntero_instruccion posicion, char** buffer) {
	int digitos = cant_digitos(posicion);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", posicion);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}
*/

/*
void serialize_puntero_char(char* etiquetas, char** buffer) {
	int digitos = cant_digitos(etiquetas);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", etiquetas);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}
*/

// SERIALIZACION INDICE DE CODIGO INSTRUCCION_DE_INICIO-INS|23-45-11-11-233-44
void serialize_t_intructions(int cantidad_instrucciones , t_intructions* instrucciones_serializado, char** buffer) {
	int i=0;

	// agrego | al comienzo para indicar comienzo de
	printf("\n%s\n",*buffer);
	(*buffer)[(strlen(*buffer)-1)] = '|';
	printf("\n%s\n",*buffer);
	//*buffer = (char * ) realloc(*buffer,(strlen(*buffer)) + 2 );

	//*buffer[]

	for(i=0;i<cantidad_instrucciones;i++){
		serialize_int(instrucciones_serializado->start,buffer);
		serialize_int(instrucciones_serializado->offset,buffer);
		instrucciones_serializado++;
	}

	printf("\n%s\n",*buffer);
	(*buffer)[(strlen(*buffer)-1)] = '|';
	printf("\n%s\n",*buffer);

}

/*
void serialize_t_size(t_size instrucciones_size, char** buffer) {
	int digitos = cant_digitos((int) instrucciones_size);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", instrucciones_size);
	strcat(*buffer,str);
	strcat(*buffer,"-");
	free(str);
}
*/


void serialize_int(int num, char** buffer) {
	int digitos = cant_digitos(num);
	char* str = malloc(digitos+1);
	snprintf(str, digitos+1, "%d", num);
	strcat((*buffer),str);
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
	char *programita = "#!/usr/bin/ansisop\nfunction imprimir\nwait mutexA\nprint $0+1\nsignal mutexB\nend\nbegin\nvariables f,  A,  g\nA = 	0\n!Global = 1+A\nprint !Global\njnz !Global\nSiguiente\n:Proximo\nf = 8\ng <- doble !Global\nio impresora 20\n:Siguiente\nimprimir A\ntextPrint    Hola Mundo!\nsumar1 &g\nprint 		g\nsinParam\nend\nfunction sinParam\ntextPrint Bye\nend\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f\nf = $0 + $0\nreturn f\nend\nfunction sumar1\n*$0 = 1 + *$0\nend\n";
	t_metadata_program *estructura = metadata_desde_literal(programita);
	char *aver_que_onda;
	char* buffer = (char*) calloc(1,SIZEOF_PCB);

	aver_que_onda = estructura->etiquetas;

	printf("\nstrlen(aver_que_onda)=%d\n",strlen(aver_que_onda));


	serialize_t_metadata_program(estructura, &buffer);
	puts(buffer);
	return 0;
}




