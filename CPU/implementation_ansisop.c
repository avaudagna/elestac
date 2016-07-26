#include "implementation_ansisop.h"
#include "libs/stack.h"

static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;

t_posicion definirVariable(t_nombre_variable variable) {

    //1) Obtener el stack index actual
    t_stack* actual_stack_index = actual_pcb->stack_index;

    //2) Obtengo la primera posicion libre del stack
    t_posicion actual_stack_pointer = (t_posicion) actual_pcb->stack_pointer;

    //3) Armamos la logical address requerida
    logical_addr* direccion_espectante = armar_direccion_logica_variable(actual_stack_pointer, setup->PAGE_SIZE);

    int valor = 0;

    t_list * pedidos = NULL;
    //TODO: CAMBIAR ESTO PARA QUE LA LISTA DE PEDIDOS SEA DE NUMEROS, NO DE SPRINTF
    obtener_lista_operaciones_escritura(&pedidos, actual_stack_pointer, ANSISOP_VAR_SIZE, valor);

    int index = 0;
    t_nodo_send * nodo = NULL;
    char * umc_response_buffer = calloc(1, sizeof(char));
    if(umc_response_buffer == NULL) {
        log_error(cpu_log, "definirVariable umc_response_buffer mem alloc failed");
        return ERROR;
    }

    char operation;
    int page, offset, size, print_index = 0;
    while (list_size(pedidos) > 0) {
        nodo = list_remove(pedidos, index);
        deserialize_data(&operation, sizeof(char), nodo->data, &print_index);
        deserialize_data(&page, sizeof(int), nodo->data, &print_index);
        deserialize_data(&offset, sizeof(int), nodo->data, &print_index);
        deserialize_data(&size, sizeof(int), nodo->data, &print_index);
        log_info(cpu_log, "Sending request op:%c (%d,%d,%d) to define variable '%c' with value %d", operation, page, offset, size, variable, valor);
        if( send(umcSocketClient, nodo->data , (size_t ) nodo->data_length, 0) < 0) {
            log_error(cpu_log, "UMC expected addr send failed");
            return ERROR;
        }
        if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) <= 0) {
            log_error(cpu_log, "UMC response recv failed");
            return ERROR;
        }
        if(*umc_response_buffer != UMC_OK_RESPONSE) {
            log_error(cpu_log, "STACK OVERFLOW");
            stack_overflow_exit();
            return STACK_OVERFLOW;
        }

    }

    list_destroy(pedidos); //Liberamos la estructura de lista reservada

    free(umc_response_buffer);

    //5) Si esta bien, agregamos la nueva t_var al stack
    t_var * nueva_variable = calloc(1, sizeof(t_var));
    nueva_variable->var_id = variable;
    nueva_variable->page_number = direccion_espectante->page_number;
    nueva_variable->offset = direccion_espectante->offset;
    nueva_variable->tamanio = direccion_espectante->tamanio;

    //6) Actualizo el stack pointer
    actual_pcb->stack_pointer += nueva_variable->tamanio;

    free(direccion_espectante);
    t_stack_entry *last_entry = get_last_entry(actual_stack_index);
    add_var( &last_entry, nueva_variable);

    //7) y retornamos la t_posicion asociada
    t_posicion posicion_nueva_variable = get_t_posicion(nueva_variable);
    return posicion_nueva_variable;
}

void stack_overflow_exit() {
    log_info(cpu_log, "=== STACK OVERFLOW EXIT ===");
    status_update(BROKEN);
}


t_posicion get_t_posicion(const t_var *variable) {
    return (t_posicion) (variable->page_number * setup->PAGE_SIZE) + variable->offset;
}

logical_addr * armar_direccion_logica_variable(int stack_index_actual, int page_size) {
    logical_addr * direccion_generada = calloc(1, sizeof(logical_addr));
    direccion_generada->page_number = stack_index_actual / page_size;
    direccion_generada->offset = stack_index_actual % page_size;
    direccion_generada->tamanio = ANSISOP_VAR_SIZE;
    return direccion_generada;

}
void obtener_lista_operaciones_escritura(t_list ** pedidos, t_posicion posicion_variable, int offset, int valor) {
    t_intructions instruccion_a_buscar;
    instruccion_a_buscar.start = (t_puntero_instruccion) posicion_variable;
    instruccion_a_buscar.offset = (t_size) offset;
    t_list * lista_direcciones = armarDireccionesLogicasList(&instruccion_a_buscar);
    armar_pedidos_escritura(pedidos, lista_direcciones, valor);
}

void armar_pedidos_escritura(t_list ** pedidos, t_list *direcciones, int valor) {
    t_nodo_send * nodo = NULL;
    int index = 0, indice_copiado = 0;
    logical_addr * address = NULL;
    *pedidos = list_create();
    char operation = ALMACENAMIENTO_BYTES;
    while(list_size(direcciones) > 0) {
        address = list_remove(direcciones, index);
        nodo = calloc(1, sizeof(t_nodo_send));
        serialize_data(&operation, sizeof(char), &nodo->data ,&nodo->data_length);
        serialize_data(&address->page_number, sizeof(int), &nodo->data ,&nodo->data_length);
        serialize_data(&address->offset, sizeof(int), &nodo->data ,&nodo->data_length);
        serialize_data(&address->tamanio, sizeof(int), &nodo->data ,&nodo->data_length);
        serialize_data( ((char*) (&valor)) + indice_copiado, (size_t) address->tamanio, &nodo->data, &nodo->data_length);
        indice_copiado += address->tamanio;
        list_add(*pedidos,nodo);
    }
}


t_posicion obtenerPosicionVariable(t_nombre_variable variable) {

    logical_addr * direccion_logica = NULL;
	int i = 0;
    //1) Obtener el stack index actual
    t_stack_entry* current_stack_index = get_last_entry(actual_pcb->stack_index);
    //2) Obtener puntero a las variables
    t_var* indice_variable = current_stack_index->vars;

    for (i = 0; i < current_stack_index->cant_vars ; i++) {
        if( (indice_variable + i)->var_id == variable ) {
            return get_t_posicion(indice_variable + i); // (t_posicion) (indice_variable->page_number * setup->PAGE_SIZE) + indice_variable->offset;
        }
    }

    return ERROR;
}

t_valor_variable dereferenciar(t_posicion direccion_variable) {
    //Hacemos el request a la UMC con el codigo 2
    t_list *pedidos = NULL;
    construir_operaciones_lectura(&pedidos, direccion_variable);

    void * variable_buffer = calloc(1, ANSISOP_VAR_SIZE), *umc_request_buffer = NULL;
    int variable_buffer_index = 0;
    char response_status;
    logical_addr * current_address = NULL;
    while(list_size(pedidos) > 0) {
        current_address  = list_remove(pedidos, 0);
//      asprintf(&umc_request_buffer, "3%04d%04d%04d", current_address->page_number,
//                 current_address->offset, current_address->tamanio);
        int umc_request_buffer_index = 0;
        char operation = PEDIDO_BYTES;
        serialize_data(&operation, sizeof(char), &umc_request_buffer, &umc_request_buffer_index);
        serialize_data(&current_address->page_number, sizeof(int), &umc_request_buffer, &umc_request_buffer_index);
        serialize_data(&current_address->offset, sizeof(int), &umc_request_buffer, &umc_request_buffer_index);
        serialize_data(&current_address->tamanio, sizeof(int), &umc_request_buffer, &umc_request_buffer_index);
        log_info(cpu_log, "sending request : op:%c (%d,%d,%d) for direccion variable %d",operation, current_address->page_number,
                 current_address->offset, current_address->tamanio, direccion_variable);
//        log_info(cpu_log, "sending request : %s for direccion variable %d", &umc_request_buffer, direccion_variable);
        if( send(umcSocketClient, umc_request_buffer, (size_t) umc_request_buffer_index, 0) < 0) {
            log_error(cpu_log, "UMC expected addr send failed");
            return ERROR;
        }

        response_status = recv_umc_response_status();
        if(response_status == '1') {
            log_info(cpu_log, "UMC_RESPONSE_OK");
        } else if (response_status == '2') {
            log_error(cpu_log, "=== STACK OVERFLOW ===");
            exit(1);
        }
        if( recv(umcSocketClient , ((char*) variable_buffer) + variable_buffer_index , (size_t) current_address->tamanio, 0) <= 0) {
            log_error(cpu_log, "UMC response recv failed");
            break;
        }
        variable_buffer_index += current_address->tamanio;
    }

	free(umc_request_buffer);
    log_info(cpu_log, "Valor variable obtained : %d", *(int*)variable_buffer);
    return (t_valor_variable) *(t_valor_variable*)variable_buffer;
}

char recv_umc_response_status() {
    void * umc_response_status_buffer = calloc(1, sizeof(char));
    char umc_response_status;
    int umc_response_status_buffer_index = 0;
    if( recv(umcSocketClient , umc_response_status_buffer, sizeof(char), 0) <= 0) {
        log_error(cpu_log, "UMC bytes recv failed");
        return ERROR;
    }
    deserialize_data(&umc_response_status, sizeof(char), umc_response_status_buffer, &umc_response_status_buffer_index);
    free(umc_response_status_buffer);
    return umc_response_status;
}
void construir_operaciones_lectura(t_list **pedidos, t_posicion posicion_variable) {
    t_intructions instruccion_a_buscar;
    instruccion_a_buscar.start = (t_puntero_instruccion) posicion_variable;
    instruccion_a_buscar.offset = (size_t) ANSISOP_VAR_SIZE;
    *pedidos = armarDireccionesLogicasList(&instruccion_a_buscar);
}

void asignar(t_posicion direccion_variable, t_valor_variable valor) {
    t_list * pedidos = NULL;
    obtener_lista_operaciones_escritura(&pedidos, direccion_variable, ANSISOP_VAR_SIZE, valor);

    char * umc_response_buffer = calloc(1, sizeof(char));
    if(umc_response_buffer == NULL) {
        log_error(cpu_log, "asignar umc_response_buffer mem alloc failed");
        return;
    }

    int index = 0;
    t_nodo_send * nodo = NULL;
    int page, offset, size, print_index = 0;
    char operation;
    while (list_size(pedidos) > 0) {
        nodo = list_remove(pedidos, index);
        if(nodo != NULL) {
            deserialize_data(&operation, sizeof(char), nodo->data, &print_index);
            deserialize_data(&page, sizeof(int), nodo->data, &print_index);
            deserialize_data(&offset, sizeof(int), nodo->data, &print_index);
            deserialize_data(&size, sizeof(int), nodo->data, &print_index);
            log_info(cpu_log, "Sending request op:%c (%d,%d,%d) for direccion variable %d with value %d", operation, page, offset, size, direccion_variable, valor);
//            log_info(cpu_log, "sending request : %s for direccion variable %d with value %d",  nodo->data , direccion_variable, valor);
            if( send(umcSocketClient, nodo->data , (size_t ) nodo->data_length, 0) < 0) {
                log_error(cpu_log, "UMC expected addr send failed");
                break;
            }

            //Obtenemos la respuesta de la UMC de un byte
            if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) <= 0) {
                log_error(cpu_log, "UMC response recv failed");
                break;
            }

            //StackOverflow: 3
            if(*umc_response_buffer == STACK_OVERFLOW_ID){
                log_error(cpu_log, "UMC raised Exception: STACKOVERFLOW");
                break;
            }

            //EXITO (Se podria loggear de que la operacion fue exitosa)
            if(*umc_response_buffer == OPERACION_EXITOSA_ID){
                log_info(cpu_log, "Asignar variable operation to UMC with value %d was successful", valor);
            } else {
                log_error(cpu_log, "Asignar variable operation to UMC with value %d failed", valor);
                break;
            }
            free(nodo);
        }
    }
    list_destroy(pedidos); //Liberamos la estructura de lista reservada
	free(umc_response_buffer);
	log_info(cpu_log, "Finished asignar variable operations");
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
    //5 + 0 + nameSize + name (1+1+4+nameSize bytes)
    void* buffer = NULL;
    int value = 0, variable_name_length = (int) strlen(variable), buffer_index = 0;
    char operation = SHARED_VAR_ID, action = GET_VAR;

//    asprintf(&buffer, "%d%d%04d%s", atoi(SHARED_VAR_ID), atoi(GET_VAR), strlen(variable), variable);
    serialize_data(&operation, sizeof(char), &buffer, &buffer_index);
    serialize_data(&action, sizeof(char), &buffer, &buffer_index);
    serialize_data(&variable_name_length, sizeof(int), &buffer, &buffer_index);
    serialize_data(variable, (size_t) variable_name_length, &buffer, &buffer_index);

    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "send of obtener variable compartida of %s failed", variable);
        free(buffer);
        return ERROR;
    }
    free(buffer);
    buffer = calloc(1, sizeof(int));
    if(recv(kernelSocketClient, buffer, sizeof(int),  0) <= 0) {
        log_error(cpu_log, "recv of obtener variable compartida failed");
        return ERROR;
    }
    int deserialize_index = 0;
    deserialize_data(&value, sizeof(int), buffer, &deserialize_index);
    log_info(cpu_log, "recv value %d of obtener variable %s", value, variable);
    free(buffer);
    return value;

}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
    //5 + 1 + nameSize + name + value (1+1+4+nameSize+4 bytes)
    void* buffer = NULL;
    int buffer_index = 0, variable_length = (int) strlen(variable);
    char operation = SHARED_VAR_ID, action = SET_VAR;
//    asprintf(&buffer, "%d%d%04d%s%04d", atoi(SHARED_VAR_ID), atoi(SET_VAR), strlen(variable), variable, valor);
    serialize_data(&operation, sizeof(char), &buffer, &buffer_index);
    serialize_data(&action, sizeof(char), &buffer, &buffer_index);
    serialize_data(&variable_length, sizeof(int), &buffer, &buffer_index);
    serialize_data(variable, (size_t) variable_length, &buffer, &buffer_index);
    serialize_data(&valor, sizeof(int), &buffer, &buffer_index);

    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "set value of %s failed", variable);
        return ERROR;
    }
    log_info(cpu_log, "asignarValorCompartida of %s with value %d was successful", variable, valor);
    free(buffer);
    return valor;
}

void irAlLabel(t_nombre_etiqueta etiqueta) {
    actual_pcb->program_counter = metadata_buscar_etiqueta(etiqueta, actual_pcb->etiquetas, (size_t) actual_pcb->etiquetas_size) - 1;
}


void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
    t_ret_var * ret_var_addr = calloc(1, sizeof(t_ret_var));
    //1) Obtengo la direccion a donde apunta la variable de retorno
    ret_var_addr = armar_direccion_logica_variable(donde_retornar, setup->PAGE_SIZE);
    t_stack_entry * actual_stack_entry = get_last_entry(actual_pcb->stack_index);
    //2) Creo la nueva entrada del stack
    t_stack_entry* new_stack_entry = create_new_stack_entry();
    //3) Guardo la posicion de PC actual como retpos de la nueva entrada
    new_stack_entry->pos = actual_stack_entry->pos + 1;
    new_stack_entry->ret_pos = actual_pcb->program_counter;
    //4) Agrego la retvar a la entrada
    add_ret_var(&new_stack_entry, ret_var_addr);
    //5) Agrego la strack entry a la queue de stack
    queue_push(actual_pcb->stack_index, new_stack_entry);
//    (t_stack_entry*) list_get(actual_pcb->stack_index->elements, 1)
    //6) Asigno el PC a la etiqueta objetivo
    irAlLabel(etiqueta);
}

void llamarSinRetorno (t_nombre_etiqueta etiqueta){
    t_ret_var * ret_var_addr = calloc(1, sizeof(t_ret_var));
    t_stack * stack_index = actual_pcb->stack_index;
    //1) Creo la nueva entrada del stack
    t_stack_entry* new_stack_entry = create_new_stack_entry();
    //2) Guardo la posicion de PC actual como retpos de la nueva entrada
    new_stack_entry->ret_pos = actual_pcb->program_counter;
    //3) Agrego la strack entry a la queue de stack
    queue_push(actual_pcb->stack_index, new_stack_entry);
    //4) Asigno el PC a la etiqueta objetivo
    irAlLabel(etiqueta);
}



void retornar(t_valor_variable retorno) {
    //1) Obtengo el stack index
    t_stack * actual_stack_index = actual_pcb->stack_index;
    //2) Obtengo la entrada actual
    t_stack_entry *last_entry = get_last_entry(actual_stack_index);
    //3) Actualizo el PC a la posicion de retorno de la entrada actual
    actual_pcb->program_counter = last_entry->ret_pos;
    //4) Asigno el valor de retorno de la entrada actual a la variable de retorno
    asignar(obtener_t_posicion(last_entry->ret_vars), retorno);
    //5) Libero la ultima entrada del indice de stack
    free(pop_stack(actual_stack_index));
}

t_posicion  obtener_t_posicion(logical_addr *address) {
    return (t_posicion) address->page_number * setup->PAGE_SIZE + address->offset;
}
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
    //cambio el estado del pcb
    status_update(BLOCKED);
    //3+ ioNameSize + ioName + io_units (1+4+ioNameSize+4 bytes)
    void * buffer = NULL;
    char operation = ENTRADA_SALIDA_ID;
    int dispositivo_length = (int) strlen(dispositivo), buffer_index = 0;
    //Armo paquete de I/O operation
//    asprintf(&buffer, "%d%04d%s%04d", atoi(ENTRADA_SALIDA_ID), strlen(dispositivo), dispositivo, tiempo);
    serialize_data(&operation, sizeof(char), &buffer, &buffer_index);
    serialize_data(&dispositivo_length, sizeof(int), &buffer, &buffer_index);
    serialize_data(dispositivo, (size_t) dispositivo_length, &buffer, &buffer_index);
    serialize_data(&tiempo, sizeof(int), &buffer, &buffer_index);

    //Envio el paquete a KERNEL
    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "entrada salida of dispositivo %s %d time send to KERNEL failed", dispositivo, tiempo);
    }
    free(buffer);
}

void imprimir(t_valor_variable valor) {
    void * buffer = NULL;
    int buffer_index = 0;
    char operation = IMPRIMIR_ID;
//    asprintf(&buffer, "%d%04d", atoi(IMPRIMIR_ID), valor);
    serialize_data(&operation , sizeof(char), &buffer, &buffer_index);
    serialize_data(&valor, sizeof(int), &buffer, &buffer_index);
    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "imprimir value %d send to KERNEL failed", valor);
        return ;
    }
    log_info(cpu_log, "imprimir value %d sent to KERNEL");
    free(buffer);
}

void imprimirTexto(char* texto) {

    void * buffer = NULL;
    int texto_len = (int) strlen(texto), buffer_index = 0;
    char operation = IMPRIMIR_TEXTO_ID;
//    asprintf(&buffer, "%d%04d%s", atoi(IMPRIMIR_TEXTO_ID), texto_len, texto);
    serialize_data(&operation, sizeof(char), &buffer, &buffer_index);
    serialize_data(&texto_len, sizeof(int), &buffer, &buffer_index);
    serialize_data(texto, (size_t) texto_len, &buffer, &buffer_index);

    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "imprimirTexto with texto : %s , send failed", texto);
        return;
    }
    log_info(cpu_log, "imprimirTexto with texto : %s , sent to KERNEL", texto);
    free(buffer);
}
void la_wait (t_nombre_semaforo identificador_semaforo){
    void * buffer = NULL, *response_buffer = calloc(1,sizeof(char));
    int buffer_index = 0, identificador_semaforo_length = (int) strlen(identificador_semaforo);
    char operation = SEMAPHORE_ID, action = WAIT_ID;
//    asprintf(&buffer, "%d%d%04d%s", atoi(SEMAPHORE_ID), atoi(WAIT_ID), strlen(identificador_semaforo), identificador_semaforo);
    serialize_data(&operation, sizeof(char), &buffer, &buffer_index);
    serialize_data(&action, sizeof(char), &buffer, &buffer_index);
    serialize_data(&identificador_semaforo_length, sizeof(int), &buffer, &buffer_index);
    serialize_data(identificador_semaforo, (size_t) identificador_semaforo_length, &buffer, &buffer_index);

    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "wait(%s) failed", identificador_semaforo);
        return;
    }
    status_update(WAITING);
    log_info(cpu_log, "Starting wait");
    //me quedo esperando activamente a que kernel me responda
//    recv(kernelSocketClient, response_buffer, sizeof(char), 0);
    //kernel_response debería ser '0'
//    log_info(cpu_log, "Finished wait");
    free(buffer);
    free(response_buffer);
}

void la_signal (t_nombre_semaforo identificador_semaforo){
    void * buffer = NULL;
    int buffer_index = 0, identificador_semaforo_length = (int) strlen(identificador_semaforo);
    char operation = SEMAPHORE_ID, action = SIGNAL_ID;
//    asprintf(&buffer, "%d%d%04d%s", atoi(SEMAPHORE_ID), atoi(SIGNAL_ID), strlen(identificador_semaforo), identificador_semaforo);
    serialize_data(&operation, sizeof(char), &buffer, &buffer_index);
    serialize_data(&action, sizeof(char), &buffer, &buffer_index);
    serialize_data(&identificador_semaforo_length, sizeof(int), &buffer, &buffer_index);
    serialize_data(identificador_semaforo, identificador_semaforo_length, &buffer, &buffer_index);

    if(send(kernelSocketClient, buffer, (size_t) buffer_index, 0) < 0) {
        log_error(cpu_log, "signal(%s) failed", identificador_semaforo);
        return;
    }
}

//En el wait me fijo si el kernel me dice de expropiarme o no, es medio choto porque no es responsabilidad del cpu de esperar estas cosas, sino del kernel.
//En el signal le digo directamente que haga signal y sigo con mi Q, porque no hay expropiacion, sino tendria que expropiarme.
//Para las variables compartidas es un simple recv o send y