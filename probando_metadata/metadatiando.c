#include <stdio.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <commons/collections/queue.h>
#include "libs/pcb.h"

t_metadata_program *getMetadataExample();
t_stack *getStackExample();

int printStackValuesVsBuffer(t_stack *stack, void *buffer);
void printStackEntryVsBuffer(t_stack_entry *entry, void *buffer, int *buffer_index);

int printInstructions(t_intructions *serializado, t_size size, void *pVoid);
int printEtiquetas(char *etiquetas, char *buffer, int cant_etiquetas);
void print_instrucciones_size ();

void printStackValuesVsStruct(t_stack *index, t_stack *stack_index);

void printStackEntryVsEntry(t_stack_entry *orig, t_stack_entry *aNew);

int main() {
    //1) Get metadata structure from ansisop whole program
    t_metadata_program* newMetadata = getMetadataExample();
    //printf("Cantidad de etiquetas: %i \n\n", newMetadata->cantidad_de_etiquetas); // print something
    //newMetadata->instrucciones_serializado += 1;

    //2) Create the PCB with the metadata information obtained, and extra information of the Kernel
    t_pcb * newPCB = malloc(sizeof(t_pcb));
    newPCB->pid=7777;
    newPCB->program_counter=newMetadata->instruccion_inicio;
    newPCB->stack_pointer=10;
    newPCB->stack_index= getStackExample();
    newPCB->status=READY; //1 = READY
    newPCB->instrucciones_size= newMetadata->instrucciones_size;
    newMetadata->instrucciones_serializado;
    newPCB->instrucciones_serializado = newMetadata->instrucciones_serializado;
    newPCB->etiquetas_size = newMetadata->etiquetas_size;
    newPCB->etiquetas = newMetadata->etiquetas;
	free(newMetadata);// let it free

    //3)Serializo el PCB recien creado
    void * pcb_buffer = NULL;
    size_t pcb_buffer_size = 0;
    serialize_pcb(newPCB, &pcb_buffer, &pcb_buffer_size);

    int auxIndex = 0;
    printf("\n===Loaded PCB vs Serialized PCB===\n");
    printf("pid: %d=%d\n", newPCB->pid, *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);
    printf("pc: %d=%d\n", newPCB->program_counter, *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);
    printf("sp: %d=%d\n", newPCB->stack_pointer, *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);
    auxIndex+= printStackValuesVsBuffer(newPCB->stack_index, pcb_buffer + auxIndex);
    printf("status: %d=%d\n", newPCB->status, *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);
    printf("instrucciones_size: %d=%d\n", newPCB->instrucciones_size, *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);
    auxIndex+= printInstructions(newPCB->instrucciones_serializado, newPCB->instrucciones_size, pcb_buffer+auxIndex);
    printf("etiquetas_size: %d=%d\n", newPCB->etiquetas_size, *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);
    auxIndex+=printEtiquetas(newPCB->etiquetas, (char*)(pcb_buffer+auxIndex),  newPCB->etiquetas_size);
    auxIndex+=newPCB->etiquetas_size;
    //4) Lo envio por socket a donde necesite ... mide pcb_buffer_size
    //free(newPCB);

    //5) Lo recibo denuevo ...

    //6) Lo deserializo
    printf("\n===Loaded PCB vs Deserialized PCB===\n");
    t_pcb * incomingPCB = (t_pcb *)calloc(1,sizeof(t_pcb));
    size_t buff_cursor = 0;
    deserialize_pcb(&incomingPCB, pcb_buffer, &buff_cursor);
    printf("pid: %d=%d\n", newPCB->pid, incomingPCB->pid);
    printf("pc: %d=%d\n", newPCB->program_counter, incomingPCB->program_counter);
    printf("sp: %d=%d\n", newPCB->stack_pointer, incomingPCB->stack_pointer);
    printStackValuesVsStruct(newPCB->stack_index, incomingPCB->stack_index);
    printf("status: %d=%d\n", newPCB->status, incomingPCB->status);
    printf("instrucciones_size: %d=%d\n", newPCB->instrucciones_size, incomingPCB->instrucciones_size);
    printInstructions(newPCB->instrucciones_serializado, newPCB->instrucciones_size, incomingPCB->instrucciones_serializado);
    printf("etiquetas_size: %d=%d\n", newPCB->etiquetas_size, incomingPCB->etiquetas_size);
    printEtiquetas(newPCB->etiquetas, incomingPCB->etiquetas,  incomingPCB->etiquetas_size);
    auxIndex+=newPCB->etiquetas_size;
    free(newPCB);
    free(incomingPCB);
    //Para este punto tendria que tener en incomingPCB el PCB deserializado :)
    return 0;
}

void printStackValuesVsStruct(t_stack *orig_stack, t_stack *new_stack) {
    t_list *elementos_orig = orig_stack->elements, *elementos_new = new_stack->elements;
    t_link_element *head_orig = elementos_orig->head, *head_new = elementos_new->head;
    int cantidad_elementos_orig = elementos_orig->elements_count;
    int cantidad_elementos_new = elementos_new->elements_count;
    int indice = 0;
    t_stack_entry *entry_orig  = (t_stack_entry*) head_orig->data, *entry_new = (t_stack_entry*) head_new->data;
    printf("cantidad_entradas_stack: %d=%d\n", cantidad_elementos_orig, cantidad_elementos_new);
    if(cantidad_elementos_orig != cantidad_elementos_new) {
        printf("||ERROR||: cantidad de elementos distinta, no se pueden comparar las entradas\n.");
    }
    if(cantidad_elementos_orig == 0) {
        printf("||INFO||: No hay entradas en elestack para mostrar.");
    }
    for (indice = 0; indice < cantidad_elementos_orig; indice ++) {
        printStackEntryVsEntry(entry_orig, entry_new);
        head_orig->next;
        head_new->next;
    }

}

void printStackEntryVsEntry(t_stack_entry *entry_orig, t_stack_entry *entry_new) {
    printf("pos: %d=%d\n", entry_orig->pos, entry_new->pos);
    printf("cant_args: %d=%d\n", entry_orig->cant_args, entry_orig->cant_args );
    printf("args\n");
    printf("page_number: %d=%d\n", entry_orig->args->page_number, entry_orig->args->page_number);
    printf("offset: %d=%d\n", entry_orig->args->offset, entry_orig->args->offset);
    printf("tamanio: %d=%d\n", entry_orig->args->tamanio, entry_orig->args->tamanio);
    printf("cant_vars: %d=%d\n", entry_orig->cant_vars, entry_orig->cant_vars);
    printf("vars\n");
    printf("var_id: %d=%d\n", entry_orig->vars->var_id, entry_orig->vars->var_id);
    printf("page_number: %d=%d\n", entry_orig->vars->page_number, entry_orig->vars->page_number);
    printf("offset: %d=%d\n", entry_orig->vars->offset, entry_orig->vars->offset);
    printf("tamanio: %d=%d\n", entry_orig->vars->tamanio, entry_orig->vars->tamanio);
    printf("cant_ret_vars: %d=%d\n", entry_orig->cant_ret_vars, entry_orig->cant_ret_vars);
    printf("ret_vars\n");
    printf("page_number: %d=%d\n", entry_orig->ret_vars->page_number, entry_orig->ret_vars->page_number);
    printf("offset: %d=%d\n", entry_orig->ret_vars->offset, entry_orig->ret_vars->offset);
    printf("tamanio: %d=%d\n", entry_orig->ret_vars->tamanio, entry_orig->ret_vars->tamanio);
    printf("ret_pos: %d=%d\n", entry_orig->ret_pos, entry_orig->ret_pos);
}

int printEtiquetas(char *etiquetas, char *buffer, int cant_etiquetas) {
    int indice = 0;
    for (indice = 0; indice < cant_etiquetas; indice++)
    {
        //printf("etiquetas: %c=%c", etiquetas[indice], buffer[indice]);
        //printf("etiquetas: %c", *(etiquetas+indice));
        printf("etiquetas\n");
    }
    return indice;
}
int printInstructions(t_intructions *instrucciones_serializado, t_size cant_instrucciones, void *buffer) {
    int indice = 0, buffer_index = 0;
    for(indice = 0; indice < cant_instrucciones; indice++) {
        printf("start: %d=%d\n", (instrucciones_serializado+indice)->start, *(int *) (buffer + buffer_index));
        buffer_index += sizeof(instrucciones_serializado->start);
        printf("offset: %d=%d\n", (instrucciones_serializado+indice)->offset, *(int *) (buffer + buffer_index));
        buffer_index += sizeof(instrucciones_serializado->offset);
    }
    return buffer_index;
}

int printStackValuesVsBuffer(t_stack *stack, void *buffer) {
    t_list* elementos = stack->elements;
    t_link_element *head = stack->elements->head;
    int cantidad_elementos = elementos->elements_count;
    int indice = 0, buffer_index = 0;
    printf("cantidad_entradas_stack: %d=%d\n", cantidad_elementos, *(int*)(buffer));
    buffer_index += sizeof(int);
    for (indice = 0; indice < cantidad_elementos; indice ++) {
        printStackEntryVsBuffer((t_stack_entry *) head->data, buffer, &buffer_index);
        head = head->next;
    }
    return buffer_index;
}

void printStackEntryVsBuffer(t_stack_entry *entry, void *buffer, int *buffer_index) {
    printf("pos: %d=%d\n", entry->pos, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->pos);
    printf("cant_args: %d=%d\n", entry->cant_args, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->cant_args);
    printf("args\n");
    printf("page_number: %d=%d\n", entry->args->page_number, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->args->page_number);
    printf("offset: %d=%d\n", entry->args->offset, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->args->offset);
    printf("tamanio: %d=%d\n", entry->args->tamanio, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->args->tamanio);
    printf("cant_vars: %d=%d\n", entry->cant_vars, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->cant_vars);
    printf("vars\n");
    printf("var_id: %d=%d\n", entry->vars->var_id, *(char*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->var_id);
    printf("page_number: %d=%d\n", entry->vars->page_number, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->page_number);
    printf("offset: %d=%d\n", entry->vars->offset, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->offset);
    printf("tamanio: %d=%d\n", entry->vars->tamanio, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->tamanio);
    printf("cant_ret_vars: %d=%d\n", entry->cant_ret_vars, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->cant_ret_vars);
    printf("ret_vars\n");
    printf("page_number: %d=%d\n", entry->ret_vars->page_number, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_vars->page_number);
    printf("offset: %d=%d\n", entry->ret_vars->offset, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_vars->offset);
    printf("tamanio: %d=%d\n", entry->ret_vars->tamanio, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_vars->tamanio);
    printf("ret_pos: %d=%d\n", entry->ret_pos, *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_pos);
}

t_stack *getStackExample() {
    t_stack * stack_mock = queue_create();
    t_stack_entry * first_entry = malloc(sizeof(t_stack_entry));
    first_entry->pos = 0;
    first_entry->cant_args = 1;
    first_entry->args = malloc(sizeof(t_arg));
    first_entry->args->page_number = 1;
    first_entry->args->offset = 2;
    first_entry->args->tamanio = 3;
    first_entry->cant_vars = 1;
    first_entry->vars = malloc(sizeof(t_var));
    first_entry->vars->var_id = 4;
    first_entry->vars->page_number = 5;
    first_entry->vars->offset = 6;
    first_entry->vars->tamanio = 7;
    first_entry->cant_ret_vars = 1;
    first_entry->ret_vars = malloc(sizeof(t_ret_var));
    first_entry->ret_vars->page_number = 8;
    first_entry->ret_vars->offset = 9;
    first_entry->ret_vars->tamanio = 10;
    first_entry->ret_pos = 0;
    queue_push(stack_mock, first_entry);
    return stack_mock;
}

t_metadata_program *getMetadataExample() {
    puts("\nThis is a fake ansisop program (hardcoded)\n");
    char *programa = "begin\nvariables f, A, g \n A = 	0  \n!Global = 1+A\n  print !Global \n jnz !Global Siguiente \n:Proximo\n	 \n f = 8	   \ng <- doble !Global	\n  io impresora 20\n	:Siguiente	 \n imprimir A  \ntextPrint  Hola Mundo!  \n\n  sumar1 &g	 \n print g  \n\n  sinParam \n \nend\n\nfunction sinParam\n	textPrint Bye\nend\n\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f  \n f = $0 + $0 \n  return fvend\n\nfunction sumar1\n	*$0 = 1 + *$0\nend\n\nfunction imprimir\n  wait mutexA\n    print $0+1\n  signal mutexB\nend\n\n";
    //print_instrucciones_size ();
    return metadata_desde_literal(programa); // get metadata from the program
}

void print_instrucciones_size () {
    char * prog1 = "begin \n variables a, b, c \n a = b + 12 \n print b \n textPrint foo\n end\"";
    char * prog2 = "begin \n print b \n end";
    char * prog3 = "begin\nend";
    char * prog4 = "end";
    char * prog5 = "begin";

    t_metadata_program * meta1 = metadata_desde_literal(prog1);
    t_metadata_program * meta2 = metadata_desde_literal(prog2);
    t_metadata_program * meta3 = metadata_desde_literal(prog3);
    t_metadata_program * meta4 = metadata_desde_literal(prog4);
    t_metadata_program * meta5 = metadata_desde_literal(prog5);

    printf("instrucciones_size prog1 :%d\n", meta1->instrucciones_size);
    printf("instrucciones_size prog2 :%d\n", meta2->instrucciones_size);
    printf("instrucciones_size prog3 :%d\n", meta3->instrucciones_size);
    printf("instrucciones_size prog4 :%d\n", meta4->instrucciones_size);
    printf("instrucciones_size prog5 :%d\n", meta5->instrucciones_size);
}