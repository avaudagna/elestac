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
void serialize_saveData(SAVE_DATA var, char **output);
void serialize_int(int valor,char **output);
int  cant_digitos(int n);



int main(void){

	REQUEST_DATA buffer;
	char *output = NULL;

	buffer.pid=205;
	buffer.page=718;
	buffer.offset=596;
	buffer.size_bytes=15;

	serialize_requestData(buffer,&output);

	printf("\n%s\n",output);

	printf("\nsizeof(int)=%d\n",sizeof(int));


	free(output);

	return 0;
}

void serialize_int(int valor,char **output){

	char *aux=NULL;

	aux = *output;

	char svalor[cant_digitos(valor)+2];	// svalor[0]='-' svalor[1..CANTIDAD_DIGITOS]= svalor[CANTIDAD_DIGITOS+1]='\0'
	sprintf(svalor,"-%d",valor);

	if(*output != NULL){	// si no es la primera vez
		*output=(char *) realloc(aux,strlen(aux) + (cant_digitos(valor)+1) + 1 );
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



