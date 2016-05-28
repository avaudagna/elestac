
#include "pcbSerialization.h"
#include "pcbDeserialization.h"

int main (void) {
    char * output = malloc(sizeof(int)*2+2);
    char* programa="begin\nvariables f, A, g \n A = 	0  \n!Global = 1+A\n  print !Global \n jnz !Global Siguiente \n:Proximo\n	 \n f = 8	   \ng <- doble !Global	\n  io impresora 20\n	:Siguiente	 \n imprimir A  \ntextPrint  Hola Mundo!  \n\n  sumar1 &g	 \n print g  \n\n  sinParam \n \nend\n\nfunction sinParam\n	textPrint Bye\nend\n\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f  \n f = $0 + $0 \n  return fvend\n\nfunction sumar1\n	*$0 = 1 + *$0\nend\n\nfunction imprimir\n  wait mutexA\n    print $0+1\n  signal mutexB\nend\n\n";
    t_metadata_program* newPCB = metadata_desde_literal(programa); // get metadata from the program
    printf("Cantidad de etiquetas: %i \n\n",newPCB->cantidad_de_etiquetas);
    serialize_t_metadata_program(newPCB, &output);
    puts("Debug output");

    t_metadata_program* estructura = (t_metadata_program*) calloc(1,sizeof(t_metadata_program));
    deserialize_t_metadata_program(&estructura, output);
    puts("Debug estructura");
    return 0;
}