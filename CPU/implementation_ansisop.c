
#include "implementation_ansisop.h"

static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;


t_puntero definirVariable(t_nombre_variable variable) {

	int page_number, offset, tamanio;
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
//	//Leo el indice del stack,
//
//	//Pido a UMC espacio en memoria(Codigo: 3) con 4bytes
//	asprintf(&buffer, "3%04d%04d%04d0000", page_number, offset, tamanio);
//	if( send(umcSocketClient, buffer, 12, 0) < 0) {
//		puts("Send addr instruction_addr");
//		return 1;
//	}
//
//	recv_bytes_buffer = calloc(1, (size_t) element->tamanio);
//	if( recv(umcSocketClient , recv_bytes_buffer , (size_t ) element->tamanio , 0) < 0) {
//		log_error(cpu_log, "UMC bytes recv failed");
//		return -1;
//	}
//
//	t_var* var = (t_var*) calloc(1,sizeof(t_var));
//
//	var->var_id = variable;
//	var->page_number = page_number;
//	var->offset = offset;
//	var->tamanio = tamanio;
//
//	return POSICION_MEMORIA;

    //1) Obtener el stack index actual
    t_stack* actual_stack_index = actual_pcb->stack_index;

    //2) Obtengo la primera posicion libre del stack
    int actual_stack_pointer = actual_pcb->stack_pointer;

    //3) Armamos la logical address requerida
    logical_addr* direccion_espectante = armar_direccion_logica(actual_stack_pointer, setup->PAGE_SIZE);

    //4) Enviamos el pedido a la UMC para que nos diga si puede meter la variable ahi o no, si puede retornamos la addr.
    char value [5]= "0000", *umc_request_buffer = NULL;

	asprintf(&umc_request_buffer, "3%04d%04d%04d%s", direccion_espectante->page_number, direccion_espectante->offset, direccion_espectante->tamanio, value);
	if( send(umcSocketClient, umc_request_buffer, 12, 0) < 0) {
        log_error(cpu_log, "UMC expected addr send failed");
		return 0;
	}
    free(umc_request_buffer);

	char * umc_response_buffer = calloc(1, sizeof(char));
	if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) < 0) {
		log_error(cpu_log, "UMC response recv failed");
		return 0;
	}

    if(strncmp(umc_response_buffer, "3", sizeof(char)) != 0) {
        log_error(cpu_log, "PAGE FAULT");
        return 0;
    }
    free(umc_response_buffer);

    //5) Si esta bien, retornamos el nuevo t_var a ser agregado al stack
    t_var * nueva_variable = calloc(1, sizeof(t_var));
    nueva_variable->var_id = variable;
    nueva_variable->page_number = direccion_espectante->page_number;
    nueva_variable->offset = direccion_espectante->offset;
    nueva_variable->tamanio = direccion_espectante->tamanio;
    free(direccion_espectante);
    add_stack_variable(&actual_stack_pointer, &actual_stack_index, nueva_variable);

    return (t_puntero) actual_stack_pointer;

}

logical_addr * armar_direccion_logica(int stack_index_actual, int page_size) {
    logical_addr * direccion_generada = calloc(1, sizeof(logical_addr));
    direccion_generada->page_number = stack_index_actual / page_size;
    direccion_generada->offset = stack_index_actual % page_size;
    direccion_generada->tamanio = ANSISOP_VAR_SIZE;
    return direccion_generada;

}

int add_stack_variable(int *stack_pointer, t_stack **stack, t_var *nueva_variable) {
    t_stack_entry *last_entry = (t_stack_entry *) queue_peek(*stack);
    //Redimensionamos el espacio donde se almacenan las entradas de variables, para agregar la nueva

    void * aux_buffer = NULL;
    aux_buffer = realloc(last_entry->vars, last_entry->cant_vars*sizeof(t_var));
    if(aux_buffer == NULL) {
        log_error(cpu_log, "Error resizing last_entry vars");
        return -1;
    }
    last_entry->vars = (t_var*) aux_buffer;
    memcpy(last_entry->vars+last_entry->cant_vars, nueva_variable, sizeof(t_var));
    *stack_pointer = *stack_pointer + nueva_variable->tamanio;
    return *stack_pointer;
}



t_puntero obtenerPosicionVariable(t_nombre_variable variable) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

t_valor_variable dereferenciar(t_puntero puntero) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

void asignar(t_puntero puntero, t_valor_variable variable) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
	printf("Asignando en %d el valor %d\n", puntero, variable);
}

void imprimir(t_valor_variable valor) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
	printf("Imprimir %d\n", valor);
}

void imprimirTexto(char* texto) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
	printf("ImprimirTexto: %s", texto);
}
