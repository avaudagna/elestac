
#include "implementation_ansisop.h"

static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;


t_posicion definirVariable(t_nombre_variable variable) {

    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);

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

    //5) Si esta bien, agregamos la nueva t_var al stack
    t_var * nueva_variable = calloc(1, sizeof(t_var));
    nueva_variable->var_id = variable;
    nueva_variable->page_number = direccion_espectante->page_number;
    nueva_variable->offset = direccion_espectante->offset;
    nueva_variable->tamanio = direccion_espectante->tamanio;
    free(direccion_espectante);
    add_stack_variable(&actual_stack_pointer, &actual_stack_index, nueva_variable);

    //6) y retornamos la t_posicion asociada
    t_posicion posicion_nueva_variable = get_t_posicion(nueva_variable);
    return posicion_nueva_variable;
}

t_posicion get_t_posicion(const t_var *variable) {
    return (t_posicion) (variable->page_number * setup->PAGE_SIZE) + variable->offset;
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



t_posicion obtenerPosicionVariable(t_nombre_variable variable) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);

    logical_addr * direccion_logica = NULL;
	int i = 0;
    //1) Obtener el stack index actual
    t_stack_entry* current_stack_index = (t_stack_entry*) actual_pcb->stack_index->elements->head;
    //2) Obtener puntero a las variables
    t_var* indice_variable = current_stack_index->vars;

    for (i = 0; i < current_stack_index->cant_vars ; i++) {
        if((strcmp(&(indice_variable + i)->var_id, &variable)) == 0) {
            return (t_posicion) (indice_variable->page_number * setup->PAGE_SIZE) + indice_variable->offset;
        }
        indice_variable++;
    }

    return -1;
}

t_valor_variable dereferenciar(t_puntero puntero) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);

    //Generamos la direccion logica a partir del puntero
    logical_addr * direccion_generada = calloc(1, sizeof(logical_addr));
    obtain_Logical_Address(direccion_generada, puntero);

    //Hacemos el request a la UMC con el codigo 2
    char* umc_request_buffer = NULL;
    asprintf(&umc_request_buffer, "2%04d%04d%04d", direccion_generada->page_number, direccion_generada->offset, direccion_generada->tamanio);
	if( send(umcSocketClient, umc_request_buffer, 13, 0) < 0) {
		log_error(cpu_log, "UMC expected addr send failed");
		return 0;
	}

	free(direccion_generada);
	free(umc_request_buffer);

	//Obtenemos la respuesta de la UMC
	char * umc_response_buffer = calloc(1, sizeof(t_valor_variable));
	if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) < 0) {
		free(umc_response_buffer);
		log_error(cpu_log, "UMC response recv failed");
		return 0;
	}

	//Pagina invalida
	if(strcmp(umc_response_buffer, PAGINA_INVALIDA_ID) == 0){
		free(umc_response_buffer);
		log_error(cpu_log, "UMC raised Exception: Invalid page");
		return 0;
	}

	//EXITO
	if(strcmp(umc_response_buffer, PAGINA_VALIDA_ID) == 0){
		free(umc_response_buffer);
		umc_response_buffer = calloc(1, 4);

		if( recv(umcSocketClient , umc_response_buffer , sizeof(t_valor_variable) , 0) < 0) {
			free(umc_response_buffer);
			log_error(cpu_log, "UMC response recv failed");
			return 0;
		}

		return (t_valor_variable) atoi(umc_response_buffer);
	}

	//Error :(
	return 0;
}

void obtain_Logical_Address(logical_addr* direccion, t_puntero posicion) {
	direccion->page_number = posicion / setup->PAGE_SIZE;
	direccion->offset = posicion % setup->PAGE_SIZE;
	direccion->tamanio = ANSISOP_VAR_SIZE;
}
//t_puntero is of the same type as t_posicion
//TODO: Decide to use whether t_puntero or t_posicion in the whole file
void asignar(t_puntero puntero, t_valor_variable variable) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);

	//Generamos la direccion logica a partir del puntero
	logical_addr * direccion_generada = calloc(1, sizeof(logical_addr));
	obtain_Logical_Address(direccion_generada, puntero);

	//Hacemos el request a la UMC con el codigo 3 para almacenar los bytes
	char* umc_request_buffer = NULL;
	asprintf(&umc_request_buffer, "3%04d%04d%04d%04d", direccion_generada->page_number, direccion_generada->offset, direccion_generada->tamanio, variable);
    int request_size = sizeof(int) * 4 + sizeof(char);
    if(send(umcSocketClient, umc_request_buffer, (size_t) request_size, 0) < 0) {
		log_error(cpu_log, "UMC expected addr send failed");
		return;
	}

	free(direccion_generada);
	free(umc_request_buffer);

	//Obtenemos la respuesta de la UMC de un byte
	char * umc_response_buffer = calloc(1, sizeof(char));
	if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) < 0) {
		free(umc_response_buffer);
		log_error(cpu_log, "UMC response recv failed");
		return;
	}

	//StackOverflow: 3
	if(strcmp(umc_response_buffer, STACK_OVERFLOW_ID) == 0){
		free(umc_response_buffer);
		log_error(cpu_log, "UMC raised Exception: STACKOVERFLOW");
		return;
	}

	//EXITO (Se podria loggear de que la operacion fue exitosa)
	if(strcmp(umc_response_buffer, OPERACION_EXITOSA_ID) == 0){
        log_info(cpu_log, "asignar variable to UMC with value %d was successful", variable);
		free(umc_response_buffer);
		return;
	}

	free(umc_response_buffer);
	log_error(cpu_log, "UMC raised Exception: Unknown exception");
}

void imprimir(t_valor_variable valor) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);

    char* kernel_request_buffer = NULL;
    char* mensaje = (char*) malloc(5);

    char* kernel_buffer = NULL;
    asprintf(&kernel_buffer, "%04d", valor);
    strcpy(mensaje, "6");
    strcat(mensaje, kernel_buffer);
    if(send(kernelSocketClient, mensaje, 5, 0) < 0) {
        log_error(cpu_log, "KERNEL expected value send failed");
        return;
    }

    free(kernel_buffer);
}

void imprimirTexto(char* texto) {
    usleep((u_int32_t ) actual_kernel_data->QSleep*1000);
    char* kernel_buffer = NULL;
    int sizeMsj = strlen("7") + 5 + (int) sizeof(texto);
    char* mensaje = (char*) malloc(sizeMsj);

    asprintf(&kernel_buffer, "%04d", (int) sizeof(texto));
    strcpy(mensaje, "7");
    strcat(mensaje, kernel_buffer);
    strcat(mensaje, texto);

    if(send(kernelSocketClient, mensaje, sizeMsj, 0) < 0) {
        log_error(cpu_log, "KERNEL expected text send failed");
        return;
    }

    free(kernel_buffer);

}
