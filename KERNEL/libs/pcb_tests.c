#include "pcb_tests.h"



void testOldPCBvsNewPCB(t_pcb *newPCB, t_pcb *incomingPCB) {
    printf("\n===(2)Loaded PCB vs Deserialized PCB===\n");
    printf("pid: %d=%d\n", newPCB->pid, incomingPCB->pid);
    assert(newPCB->pid == incomingPCB->pid);
    printf("pc: %d=%d\n", newPCB->program_counter, incomingPCB->program_counter);
    assert(newPCB->program_counter == incomingPCB->program_counter);
    printf("sp: %d=%d\n", newPCB->stack_pointer, incomingPCB->stack_pointer);
    assert(newPCB->stack_pointer == incomingPCB->stack_pointer);
    printStackValuesVsStruct(newPCB->stack_index, incomingPCB->stack_index);
    printf("status: %d=%d\n", newPCB->status, incomingPCB->status);
    assert(newPCB->status == incomingPCB->status);
    printf("instrucciones_size: %d=%d\n", newPCB->instrucciones_size, incomingPCB->instrucciones_size);
    assert(newPCB->instrucciones_size == incomingPCB->instrucciones_size);
    printInstructions(newPCB->instrucciones_serializado, newPCB->instrucciones_size, incomingPCB->instrucciones_serializado);
    printf("etiquetas_size: %d=%d\n", newPCB->etiquetas_size, incomingPCB->etiquetas_size);
    assert(newPCB->etiquetas_size == incomingPCB->etiquetas_size);
    printEtiquetas(newPCB->etiquetas, incomingPCB->etiquetas,  incomingPCB->etiquetas_size);
    assert(memcmp(newPCB->etiquetas, incomingPCB->etiquetas,incomingPCB->etiquetas_size) == 0);
}
void printSerializedPcb(void *pcb_buffer) {
    int auxIndex = 0;
    printf("\n===Serialized PCB values===\n");

    printf("pid: %d\n", *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    printf("pc: %d\n", *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    printf("sp: %d\n", *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    auxIndex+= printBufferStackValues(pcb_buffer + auxIndex);

    printf("status: %d\n", *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    printf("instrucciones_size: %d\n", *(int*)(pcb_buffer+auxIndex));
    int cantidad_instrucciones = *(int*)(pcb_buffer+auxIndex);
    auxIndex+=sizeof(int);

    auxIndex+= printBufferInstructions(cantidad_instrucciones, pcb_buffer+auxIndex);

    printf("etiquetas_size: %d\n", *(int*)(pcb_buffer+auxIndex));
    int etiquetas_size = *(int*)(pcb_buffer+auxIndex);
    auxIndex+=sizeof(int);

    auxIndex+=printBufferEtiquetas((char*)(pcb_buffer+auxIndex),  etiquetas_size);
    auxIndex+=etiquetas_size;
}

void testSerializedPCB(t_pcb *newPCB, void *pcb_buffer) {
    int auxIndex = 0;
    printf("\n===(1)Loaded PCB vs Serialized PCB===\n");

    printf("pid: %d=%d\n", newPCB->pid, *(int*)(pcb_buffer+auxIndex));
    assert(newPCB->pid == *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    printf("pc: %d=%d\n", newPCB->program_counter, *(int*)(pcb_buffer+auxIndex));
    assert(newPCB->program_counter == *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    printf("sp: %d=%d\n", newPCB->stack_pointer, *(int*)(pcb_buffer+auxIndex));
    assert(newPCB->stack_pointer == *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    auxIndex+= printStackValuesVsBuffer(newPCB->stack_index, pcb_buffer + auxIndex);

    printf("status: %d=%d\n", newPCB->status, *(int*)(pcb_buffer+auxIndex));
    assert(newPCB->status == *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    printf("instrucciones_size: %d=%d\n", newPCB->instrucciones_size, *(int*)(pcb_buffer+auxIndex));
    assert(newPCB->instrucciones_size == *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    auxIndex+= printInstructions(newPCB->instrucciones_serializado, newPCB->instrucciones_size, pcb_buffer+auxIndex);

    printf("etiquetas_size: %d=%d\n", newPCB->etiquetas_size, *(int*)(pcb_buffer+auxIndex));
    assert(newPCB->etiquetas_size == *(int*)(pcb_buffer+auxIndex));
    auxIndex+=sizeof(int);

    auxIndex+=printEtiquetas(newPCB->etiquetas, (char*)(pcb_buffer+auxIndex),  newPCB->etiquetas_size);
    auxIndex+=newPCB->etiquetas_size;
    assert(memcmp(newPCB->etiquetas, (char*)(pcb_buffer+auxIndex),  newPCB->etiquetas_size) == 0);
}

void printStackValuesVsStruct(t_stack *orig_stack, t_stack *new_stack) {
    t_list *elementos_orig = orig_stack->elements, *elementos_new = new_stack->elements;
    t_link_element *head_orig = elementos_orig->head, *head_new = elementos_new->head;
    int cantidad_elementos_orig = elementos_orig->elements_count;
    int cantidad_elementos_new = elementos_new->elements_count;
    int indice = 0;
    printf("cantidad_entradas_stack: %d=%d\n", cantidad_elementos_orig, cantidad_elementos_new);
    assert(cantidad_elementos_orig == cantidad_elementos_new);
    if(cantidad_elementos_orig != cantidad_elementos_new) {
        printf("||ERROR||: cantidad de elementos distinta, no se pueden comparar las entradas\n.");
    }
    if(cantidad_elementos_orig == 0) {
        printf("||INFO||: No hay entradas en elestack para mostrar.\n");
        return;
    }
    t_stack_entry *entry_orig  = (t_stack_entry*) head_orig->data, *entry_new = (t_stack_entry*) head_new->data;

    for (indice = 0; indice < cantidad_elementos_orig; indice ++) {
        printStackEntryVsEntry(entry_orig, entry_new);
        head_orig->next;
        head_new->next;
    }

}

void printStackEntryVsEntry(t_stack_entry *entry_orig, t_stack_entry *entry_new) {
    printf("pos: %d=%d\n", entry_orig->pos, entry_new->pos);
    assert(entry_orig->pos == entry_new->pos);
    printf("cant_args: %d=%d\n", entry_orig->cant_args, entry_new->cant_args);
    assert(entry_orig->cant_args == entry_new->cant_args);
    printf("args\n");
    printf("page_number: %d=%d\n", entry_orig->args->page_number, entry_new->args->page_number);
    assert(entry_orig->args->page_number == entry_new->args->page_number);
    printf("offset: %d=%d\n", entry_orig->args->offset, entry_new->args->offset);
    assert(entry_orig->args->offset == entry_new->args->offset);
    printf("tamanio: %d=%d\n", entry_orig->args->tamanio, entry_new->args->tamanio);
    assert(entry_orig->args->tamanio == entry_new->args->tamanio);
    printf("cant_vars: %d=%d\n", entry_orig->cant_vars, entry_new->cant_vars);
    assert(entry_orig->cant_vars == entry_new->cant_vars);
    printf("vars\n");
    printf("var_id: %d=%d\n", entry_orig->vars->var_id, entry_new->vars->var_id);
    assert(entry_orig->vars->var_id == entry_new->vars->var_id);
    printf("page_number: %d=%d\n", entry_orig->vars->page_number, entry_new->vars->page_number);
    assert(entry_orig->vars->page_number == entry_new->vars->page_number);
    printf("offset: %d=%d\n", entry_orig->vars->offset, entry_new->vars->offset);
    assert(entry_orig->vars->offset == entry_new->vars->offset);
    printf("tamanio: %d=%d\n", entry_orig->vars->tamanio, entry_new->vars->tamanio);
    assert(entry_orig->vars->tamanio == entry_new->vars->tamanio);
    printf("cant_ret_vars: %d=%d\n", entry_orig->cant_ret_vars, entry_new->cant_ret_vars);
    assert(entry_orig->cant_ret_vars == entry_new->cant_ret_vars);
    printf("ret_vars\n");
    printf("page_number: %d=%d\n", entry_orig->ret_vars->page_number, entry_new->ret_vars->page_number);
    assert(entry_orig->ret_vars->page_number == entry_new->ret_vars->page_number);
    printf("offset: %d=%d\n", entry_orig->ret_vars->offset, entry_new->ret_vars->offset);
    assert(entry_orig->ret_vars->offset == entry_new->ret_vars->offset);
    printf("tamanio: %d=%d\n", entry_orig->ret_vars->tamanio, entry_new->ret_vars->tamanio);
    assert(entry_orig->ret_vars->tamanio == entry_new->ret_vars->tamanio);
    printf("ret_pos: %d=%d\n", entry_orig->ret_pos, entry_new->ret_pos);
    assert(entry_orig->ret_pos == entry_new->ret_pos);
}

int printBufferEtiquetas(char *buffer, int cant_etiquetas) {
    int indice = 0;
    for (indice = 0; indice < cant_etiquetas; indice++)
    {
        //printf("etiquetas: %c=%c", etiquetas[indice], buffer[indice]);
        //printf("etiquetas: %c", *(etiquetas+indice));
        printf("etiquetas\n");
    }
    return indice;
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

int printBufferInstructions(int cant_instrucciones, void *buffer) {
    int indice = 0, buffer_index = 0;
    for(indice = 0; indice < cant_instrucciones; indice++) {
        printf("start: %d\n", *(int *) (buffer + buffer_index));
        buffer_index += sizeof(int);

        printf("offset: %d\n", *(int *) (buffer + buffer_index));
        buffer_index += sizeof(int);
    }
    return buffer_index;
}

int printInstructions(t_intructions *instrucciones_serializado, int cant_instrucciones, void *buffer) {
    int indice = 0, buffer_index = 0;
    for(indice = 0; indice < cant_instrucciones; indice++) {
        printf("start: %d=%d\n", (instrucciones_serializado+indice)->start, *(int *) (buffer + buffer_index));
        assert((instrucciones_serializado+indice)->start == *(int *) (buffer + buffer_index));
        buffer_index += sizeof(instrucciones_serializado->start);

        printf("offset: %d=%d\n", (instrucciones_serializado+indice)->offset, *(int *) (buffer + buffer_index));
        assert((instrucciones_serializado+indice)->offset == *(int *) (buffer + buffer_index));
        buffer_index += sizeof(instrucciones_serializado->offset);
    }
    return buffer_index;
}

int printBufferStackValues( void *buffer) {
    int cantidad_elementos = 0, indice = 0, buffer_index = 0;

    printf("cantidad_entradas_stack: %d\n", *(int*)(buffer));
    cantidad_elementos = *(int*)(buffer);
    buffer_index += sizeof(int);

    for (indice = 0; indice < cantidad_elementos; indice ++) {
        printBufferStackEntry(buffer, &buffer_index);
    }
    return buffer_index;
}

int printStackValuesVsBuffer(t_stack *stack, void *buffer) {
    t_list* elementos = stack->elements;
    t_link_element *head = stack->elements->head;
    int cantidad_elementos = elementos->elements_count;
    int indice = 0, buffer_index = 0;

    printf("cantidad_entradas_stack: %d=%d\n", cantidad_elementos, *(int*)(buffer));
    assert(cantidad_elementos == *(int*)(buffer));
    buffer_index += sizeof(int);

    for (indice = 0; indice < cantidad_elementos; indice ++) {
        printStackEntryVsBuffer((t_stack_entry *) head->data, buffer, &buffer_index);
        head = head->next;
    }
    return buffer_index;
}
void printBufferStackEntry(void *buffer, int *buffer_index) {
    printf("pos: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("cant_args: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("args\n");
    printf("page_number: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("offset: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("tamanio: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("cant_vars: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("vars\n");
    printf("var_id: %d\n", *(char*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("page_number: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("offset: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("tamanio: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("cant_ret_vars: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("ret_vars\n");
    printf("page_number: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("offset: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("tamanio: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);

    printf("ret_pos: %d\n", *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(int);
}

void printStackEntryVsBuffer(t_stack_entry *entry, void *buffer, int *buffer_index) {
    printf("pos: %d=%d\n", entry->pos, *(int*)(buffer+ *buffer_index));
    assert(entry->pos == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->pos);

    printf("cant_args: %d=%d\n", entry->cant_args, *(int*)(buffer+ *buffer_index));
    assert( entry->cant_args == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->cant_args);

    printf("args\n");
    printf("page_number: %d=%d\n", entry->args->page_number, *(int*)(buffer+ *buffer_index));
    assert(entry->args->page_number == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->args->page_number);

    printf("offset: %d=%d\n", entry->args->offset, *(int*)(buffer+ *buffer_index));
    assert( entry->args->offset == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->args->offset);

    printf("tamanio: %d=%d\n", entry->args->tamanio, *(int*)(buffer+ *buffer_index));
    assert( entry->args->tamanio == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->args->tamanio);

    printf("cant_vars: %d=%d\n", entry->cant_vars, *(int*)(buffer+ *buffer_index));
    assert(entry->cant_vars == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->cant_vars);

    printf("vars\n");
    printf("var_id: %d=%d\n", entry->vars->var_id, *(char*)(buffer+ *buffer_index));
    assert(entry->vars->var_id == *(char*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->var_id);

    printf("page_number: %d=%d\n", entry->vars->page_number, *(int*)(buffer+ *buffer_index));
    assert(entry->vars->page_number ==*(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->page_number);

    printf("offset: %d=%d\n", entry->vars->offset, *(int*)(buffer+ *buffer_index));
    assert(entry->vars->offset == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->offset);

    printf("tamanio: %d=%d\n", entry->vars->tamanio, *(int*)(buffer+ *buffer_index));
    assert(entry->vars->tamanio == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->vars->tamanio);

    printf("cant_ret_vars: %d=%d\n", entry->cant_ret_vars, *(int*)(buffer+ *buffer_index));
    assert(entry->cant_ret_vars == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->cant_ret_vars);

    printf("ret_vars\n");
    printf("page_number: %d=%d\n", entry->ret_vars->page_number, *(int*)(buffer+ *buffer_index));
    assert(entry->ret_vars->page_number == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_vars->page_number);

    printf("offset: %d=%d\n", entry->ret_vars->offset, *(int*)(buffer+ *buffer_index));
    assert(entry->ret_vars->offset == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_vars->offset);

    printf("tamanio: %d=%d\n", entry->ret_vars->tamanio, *(int*)(buffer+ *buffer_index));
    assert(entry->ret_vars->tamanio == *(int*)(buffer+ *buffer_index));
    *buffer_index += sizeof(entry->ret_vars->tamanio);

    printf("ret_pos: %d=%d\n", entry->ret_pos, *(int*)(buffer+ *buffer_index));
    assert(entry->ret_pos == *(int*)(buffer+ *buffer_index));
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