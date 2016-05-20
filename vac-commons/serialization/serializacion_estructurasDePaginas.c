/*
 * main.c
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct req_data {
	int     	pid,
		   	   page,
	         offset,
         size_bytes;

}REQUEST_DATA;


typedef struct save_data {
	int     	pid,
		   	   page,
	         offset,
         size_bytes;
	void 	  *data;

}SAVE_DATA;


void serialize_requestData(REQUEST_DATA var,char **output);
void deserialize_requestData(REQUEST_DATA *var,char *estructura_serializada);
void serialize_saveData(SAVE_DATA var, char **output);
void serialize_data(void * data,int size_bytes,char **output);
//void deserialize_saveData(SAVE_DATA *var,char *estructura_serializada);
void serialize_int(int valor,char **output);

int  cant_digitos(int n);



int main(void){

	int i=0;
	char *output = NULL;
	char *aux = "SEÑORAS Y SEÑORES , TODO BIEN POR ACA??";

	SAVE_DATA estructurita;

	estructurita.pid=15;
	estructurita.page=43;
	estructurita.offset=6;
	estructurita.data = aux;
	estructurita.size_bytes = strlen(aux);		// sin el \0

	serialize_saveData(estructurita,&output);

	printf("\n strlen(output)=%d\n",strlen(output));

	for(i=0;i<strlen(output);i++){
		printf("%c",output[i]);
	}


	free(output);

	return 0;
}





void serialize_int(int valor,char **output){

	if(*output != NULL){	// si no es la primera vez

		char svalor[cant_digitos(valor)+2];	// svalor[0]='-' svalor[1..CANTIDAD_DIGITOS]= svalor[CANTIDAD_DIGITOS+1]='\0'
		sprintf(svalor,"-%d",valor);
		*output=(char *) realloc(*output,strlen(*output) + (cant_digitos(valor)+1) + 1 );
		strcat(*output,svalor);

	}
	else{	// si es la primera vez
		*output=(char *) realloc(*output,sizeof(*output) + (cant_digitos(valor)+1) );
		sprintf(*output,"%d",valor);
	}
}

void serialize_requestData(REQUEST_DATA var,char **output){

	serialize_int(var.pid,output);
	serialize_int(var.page,output);
	serialize_int(var.offset,output);
	serialize_int(var.size_bytes,output);

}

void serialize_saveData(SAVE_DATA var, char **output){
	serialize_int(var.pid,output);
	serialize_int(var.page,output);
	serialize_int(var.offset,output);
	serialize_int(var.size_bytes,output);
	serialize_data(var.data,var.size_bytes,output);
	// falta como hacer para el DATO propiamente dicho... void * ..??
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


void deserialize_requestData(REQUEST_DATA *var,char *estructura_serializada){


}



void serialize_data(void * data,int size_bytes,char ** output){

	// agrando todo el stream serializado para poder concatener data
	*output=(char *) realloc(*output,strlen(*output) + size_bytes );
	// copio la data
	memcpy((*output)+strlen(*output),data,size_bytes);

}

