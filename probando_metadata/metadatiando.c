#include <stdio.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "libs/pcb.h"


t_metadata_program *getMetadataExample();

int main() {
    //1) Get metadata structure from ansisop whole program
    t_metadata_program* newMetadata = getMetadataExample();
    //printf("Cantidad de etiquetas: %i \n\n", newMetadata->cantidad_de_etiquetas); // print something
    //newMetadata->instrucciones_serializado += 1;

    //2) Create the PCB with the metadata information obtained, and extra information of the Kernel
    t_pcb * newPCB = malloc(sizeof(t_pcb));
    newPCB->pid=atoi("7777");
    newPCB->program_counter=newMetadata->instruccion_inicio;
    newPCB->stack_pointer=10;
    newPCB->stack_index=queue_create();
    newPCB->status=NEW;
    newPCB->instrucciones_size= newMetadata->instrucciones_size;
    newMetadata->instrucciones_serializado;
    char * instrucciones_buffer = NULL;
    t_size instrucciones_buffer_size = 0;
    serialize_instrucciones(newMetadata->instrucciones_serializado, &instrucciones_buffer, &instrucciones_buffer_size) ;
    newPCB->instrucciones_serializado = instrucciones_buffer;
	free(newMetadata);// let it free

    //3)Serializo el PCB recien creado
    char * pcb_buffer = NULL;
    t_size pcb_buffer_size = 0;
    serialize_pcb(newPCB, &pcb_buffer, &pcb_buffer_size);
    free(newPCB);

    //4) Lo envio por socket a donde necesite ... mide pcb_buffer_size

    //5) Lo recibo denuevo ...

    //6) Lo deserializo
    t_pcb * incomingPCB = NULL;
    deserialize_pcb(&incomingPCB, &pcb_buffer, &pcb_buffer_size);

    //Para este punto tendria que tener en incomingPCB el PCB deserializado :)
}

t_metadata_program *getMetadataExample() {
    puts("\nThis is a fake ansisop program (hardcoded)\n");
    char *programa = "begin\nvariables f, A, g \n A = 	0  \n!Global = 1+A\n  print !Global \n jnz !Global Siguiente \n:Proximo\n	 \n f = 8	   \ng <- doble !Global	\n  io impresora 20\n	:Siguiente	 \n imprimir A  \ntextPrint  Hola Mundo!  \n\n  sumar1 &g	 \n print g  \n\n  sinParam \n \nend\n\nfunction sinParam\n	textPrint Bye\nend\n\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f  \n f = $0 + $0 \n  return fvend\n\nfunction sumar1\n	*$0 = 1 + *$0\nend\n\nfunction imprimir\n  wait mutexA\n    print $0+1\n  signal mutexB\nend\n\n";
    return metadata_desde_literal(programa); // get metadata from the program
}
