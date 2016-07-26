/*
 * metadatiando.c
 *
 *  Created on: 14/5/2016
 *      Author: utnso
 */
#include <parser.h>
#include <metadata_program.h>
#include <sintax.h>
#include <stdlib.h>


void CopiarDesdeHasta(char * code,u_int32_t start ,u_int32_t end,int nro_inst);

int main()
{
	t_metadata_program * newPCB;
	t_intructions * aux= NULL;
	//char *buffer;
	int cantidad_de_instrucciones = 0 , i=0;


	puts("\nThis is a fake ansisop program (hardcoded)\n");
	char* programa="begin\nvariables f, A, g \n A = 	0  \n!Global = 1+A\n  print !Global \n jnz !Global Siguiente \n:Proximo\n	 \n f = 8	   \ng <- doble !Global	\n  io impresora 20\n	:Siguiente	 \n imprimir A  \ntextPrint  Hola Mundo!  \n\n  sumar1 &g	 \n print g  \n\n  sinParam \n \nend\n\nfunction sinParam\n	textPrint Bye\nend\n\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f  \n f = $0 + $0 \n  return fvend\n\nfunction sumar1\n	*$0 = 1 + *$0\nend\n\nfunction imprimir\n  wait mutexA\n    print $0+1\n  signal mutexB\nend\n\n";
	newPCB = metadata_desde_literal(programa); // get metadata from the program
	cantidad_de_instrucciones = newPCB->instrucciones_size;
	aux = newPCB->instrucciones_serializado;

	for (i=0;i<cantidad_de_instrucciones;i++){
		CopiarDesdeHasta(programa,aux->start,aux->offset,i);
		aux++;
	}

	printf("Cantidad de etiquetas: %i \n\n",newPCB->cantidad_de_etiquetas); // print something

	puts("Debugueame aca");
	//free(programa); // let it free

	return 0;
}

void CopiarDesdeHasta(char * code,u_int32_t  start ,u_int32_t offset,int nro_inst){
	char instruccion[offset+1];
	int  i = 0,
		 j = 0;

	//buffer = (char*) malloc(sizeof(char) * (strlen(code)+1));
	//strcpy(buffer,code);

	//instruccion = (char *) malloc(sizeof(char) * offset );
	//instruccion = (char *) malloc((sizeof(char)) * offset);
	//buffer[(start)-1] = '|';
	//buffer[(end)-1] = '|';

	for ( i=start,j=0;j<offset;i++,j++)
	{
		instruccion[j] = code[i] ;
	}

	instruccion[j]='\0';

	printf("\nInstruccion n°%d , contiene el siguiente codigo :%s",nro_inst,instruccion);

	//instruccion = strtok(buffer,"|");
	//free(buffer);

	//free(instruccion);

}
