#include <parser/metadata_program.h>
#include "implementation_ansisop.h"
#include "libs/stack.h"
#include "libs/pcb.h"
#include "cpu_structs.h"

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
    obtener_lista_operaciones_escritura(&pedidos, actual_stack_pointer, ANSISOP_VAR_SIZE, valor);

    int index = 0;
    t_nodo_send * nodo = NULL;
    char * umc_response_buffer = calloc(1, sizeof(char));
    if(umc_response_buffer == NULL) {
        log_error(cpu_log, "definirVariable umc_response_buffer mem alloc failed");
        return ERROR;
    }

    while (list_size(pedidos) > 0) {
        nodo = list_remove(pedidos, index);
        log_info(cpu_log, "sending request : %s for variable %c with value %d", nodo->data, variable, valor);
        if( send(umcSocketClient, nodo->data , (size_t ) nodo->data_length, 0) < 0) {
            log_error(cpu_log, "UMC expected addr send failed");
            return ERROR;
        }
        if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) < 0) {
            log_error(cpu_log, "UMC response recv failed");
            return ERROR;
        }
        if(strncmp(umc_response_buffer, string_itoa(UMC_OK_RESPONSE), sizeof(char)) != 0) {
            log_error(cpu_log, "PAGE FAULT");
            return ERROR;
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
    t_stack_entry *last_entry = (t_stack_entry *) queue_peek(actual_stack_index);
    add_var( &last_entry, nueva_variable);

    //7) y retornamos la t_posicion asociada
    t_posicion posicion_nueva_variable = get_t_posicion(nueva_variable);
    return posicion_nueva_variable;
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
    void *buffer = NULL;
    t_nodo_send * nodo = NULL;
    int index = 0, indice_copiado = 0;
    logical_addr * address = NULL;
    int buff_len = 0;
    *pedidos = list_create();
    while(list_size(direcciones) > 0) {
        address = list_remove(direcciones, index);
        nodo = calloc(1, sizeof(t_nodo_send));
        buff_len = sizeof(char) + sizeof(int) *3;
        buffer = calloc(1, sizeof(char) + sizeof(int) *3 + address->tamanio);
        nodo->data = buffer;
        sprintf(buffer, "4%04d%04d%04d", address->page_number, address->offset, address->tamanio);
        serialize_data( ((char*) (&valor)) + indice_copiado, (size_t) address->tamanio, &buffer, &buff_len);
        indice_copiado += address->tamanio;
        nodo->data_length = buff_len;
        list_add(*pedidos,nodo);
    }
}


t_posicion obtenerPosicionVariable(t_nombre_variable variable) {

    logical_addr * direccion_logica = NULL;
	int i = 0;
    //1) Obtener el stack index actual
    t_stack_entry* current_stack_index = (t_stack_entry*) queue_peek(actual_pcb->stack_index);
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

    char* umc_request_buffer = NULL;
    char * umc_response_buffer = NULL;
    void * variable_buffer = calloc(1, ANSISOP_VAR_SIZE);
    int variable_buffer_index = 0;
    logical_addr * current_address = NULL;
    while(list_size(pedidos) > 0) {
        current_address  = list_remove(pedidos, 0);
        umc_response_buffer = calloc(1, sizeof(t_valor_variable));
        asprintf(&umc_request_buffer, "3%04d%04d%04d", current_address->page_number, current_address->offset, current_address->tamanio);
        log_info(cpu_log, "sending request : %s for direccion variable %d", umc_request_buffer, direccion_variable);
        if( send(umcSocketClient, umc_request_buffer, 13, 0) < 0) {
            log_error(cpu_log, "UMC expected addr send failed");
            return ERROR;
        }
        if( recv(umcSocketClient , ((char*) variable_buffer) + variable_buffer_index , (size_t) current_address->tamanio , 0) < 0) {
            log_error(cpu_log, "UMC response recv failed");
            break;
        }
        variable_buffer_index += current_address->tamanio;
    }

	free(umc_request_buffer);
    return (t_valor_variable) *(t_valor_variable*)variable_buffer;
}

void construir_operaciones_lectura(t_list **pedidos, t_posicion posicion_variable) {
    t_intructions instruccion_a_buscar;
    instruccion_a_buscar.start = (t_puntero_instruccion) posicion_variable;
    instruccion_a_buscar.offset = (t_size) ANSISOP_VAR_SIZE;
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
    while (list_size(pedidos) > 0) {
        nodo = list_remove(pedidos, index);
        if(nodo != NULL) {
            log_info(cpu_log, "sending request : %s for direccion variable %d with value %d",  nodo->data , direccion_variable, valor);
            if( send(umcSocketClient, nodo->data , (size_t ) nodo->data_length, 0) < 0) {
                log_error(cpu_log, "UMC expected addr send failed");
                break;
            }

            //Obtenemos la respuesta de la UMC de un byte
            if( recv(umcSocketClient , umc_response_buffer , sizeof(char) , 0) < 0) {
                log_error(cpu_log, "UMC response recv failed");
                break;
            }

            //StackOverflow: 3
            if(strcmp(umc_response_buffer, STACK_OVERFLOW_ID) == 0){
                log_error(cpu_log, "UMC raised Exception: STACKOVERFLOW");
                break;
            }

            //EXITO (Se podria loggear de que la operacion fue exitosa)
            if(strcmp(umc_response_buffer, OPERACION_EXITOSA_ID) == 0){
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
    char* buffer = NULL;
    char *value = malloc(4);

    int buffer_size = sizeof(char) * 2 + 4 + strlen(variable);
    asprintf(&buffer, "%d%d%04d%s", SHARED_VAR_ID, GET_VAR, strlen(variable), variable);
    if(send(kernelSocketClient, buffer, (size_t) buffer_size, 0) < 0) {
        log_error(cpu_log, "get value of %s failed", variable);
        return ERROR;
    }

    recv(kernelSocketClient, value, 4,  0);

    free(buffer);
    free(value);

    return (int) atoi(value);

}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
    //5 + 1 + nameSize + name + value (1+1+4+nameSize+4 bytes)

    char* buffer = NULL;
    char *kernel_reply = malloc(1);

    int buffer_size = sizeof(char) * 2 + 8 + strlen(variable);
    asprintf(&buffer, "%d%d%04d%s%04d", SHARED_VAR_ID, SET_VAR, strlen(variable), variable, value);
    if(send(kernelSocketClient, buffer, (size_t) buffer_size, 0) < 0) {
        log_error(cpu_log, "set value of %s failed", variable);
        return ERROR;
    }

    if( recv(kernelSocketClient , kernel_reply , sizeof(char) , 0) < 0) {
        log_error(cpu_log, "Kernel response recv failed");
        return ERROR;
    }
    if(!strncmp(kernel_reply, "1")) {
        log_error(cpu_log, "No se pudo asignar el valor %d a la variable %s", valor, variable);
        return ERROR;
    }else if(!strncmp(kernel_reply, "0")){
        return valor;
    }
    free(buffer);
    free(kernel_reply);

}

void irAlLabel(t_nombre_etiqueta etiqueta) {
    actual_pcb->program_counter = metadata_buscar_etiqueta(etiqueta, actual_pcb->etiquetas, (const t_size) actual_pcb->etiquetas_size);
}


void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
    t_ret_var * ret_var_addr = calloc(1, sizeof(t_ret_var));
    //1) Obtengo la direccion a donde apunta la variable de retorno
    ret_var_addr = armar_direccion_logica_variable(donde_retornar, setup->PAGE_SIZE);
    t_stack * stack_index = actual_pcb->stack_index;
    //2) Creo la nueva entrada del stack
    t_stack_entry* new_stack_entry = create_new_stack_entry();
    //3) Guardo la posicion de PC actual como retpos de la nueva entrada
    new_stack_entry->ret_pos = actual_pcb->program_counter;
    //4) Agrego la retvar a la entrada
    add_ret_var(&new_stack_entry, ret_var_addr);
    //5) Agrego la strack entry a la queue de stack
    queue_push(actual_pcb->stack_index, new_stack_entry);
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
    t_stack_entry *last_entry = (t_stack_entry *) queue_peek(actual_stack_index);
    //3) Actualizo el PC a la posicion de retorno de la entrada actual
    actual_pcb->program_counter = last_entry->ret_pos;
    //4) Asigno el valor de retorno de la entrada actual a la variable de retorno
    asignar(obtener_t_posicion(last_entry->ret_vars), retorno);
    //5) Libero la ultima entrada del indice de stack
    queue_pop(actual_stack_index);
}

t_posicion  obtener_t_posicion(logical_addr *address) {
    return (t_posicion) address->page_number * setup->PAGE_SIZE + address->offset;
}
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {

    //cambio el estado del pcb
    actual_pcb->status = BLOCKED;

    //NO VA MÁS serialize_pcb(actual_pcb, &buffer, &buffer_len);
    //3+ ioNameSize + ioName + io_units (1+4+ioNameSize+4 bytes)

    char* mensaje = NULL;
    char* buffer = NULL;

    int sizeMsj = sizeof(char) + sizeof(ioName) + 8;
    //Armo paquete de I/O operation
    asprintf(&buffer, "%d%04d", atoi(ENTRADA_SALIDA_ID), strlen(dispositivo));
    strcpy(mensaje, buffer);
    strcat(mensaje, dispositivo);
    asprintf(&buffer, "%04d", tiempo);
    strcat(mensaje, buffer);

    free(buffer);

    //Envio el paquete a KERNEL
    if(send(kernelSocketClient, mensaje, (size_t) sizeMsj, 0) < 0) {
        log_error(cpu_log, "entrada salida of dispositivo %s %d time send to KERNEL failed", dispositivo, tiempo);
    }

    free(mensaje);

}

int imprimir(t_valor_variable valor) {
    char* buffer = NULL;
    int buffer_size = sizeof(char) + sizeof(int);
    asprintf(&buffer, "%d%04d", IMPRIMIR_ID, valor);
    if(send(kernelSocketClient, buffer, (size_t) buffer_size, 0) < 0) {
        log_error(cpu_log, "imprimir value %d send to KERNEL failed", valor);
        return ERROR;
    }
    free(buffer);
    return (int) strlen(string_itoa(valor));
}

int imprimirTexto(char* texto) {

    char* buffer = NULL;
    int texto_len = (int) strlen(texto);
    asprintf(&buffer, "%d%04d%s", IMPRIMIR_TEXTO_ID, texto_len, texto);
    int buffer_size = (int) strlen(buffer);

    if(send(kernelSocketClient, buffer, (t_size) buffer_size, 0) < 0) {
        log_error(cpu_log, "imprimirTexto send failed");
        return ERROR;
    }
    free(buffer);
    return texto_len;
}
void wait(t_nombre_semaforo identificador_semaforo){
    char* buffer = NULL;
    int buffer_size = sizeof(char) * 2 + 4 + strlen(identificador_semaforo);
    asprintf(&buffer, "%d%d%04d%s", SEMAPHORE_ID, WAIT_ID, strlen(identificador_semaforo), identificador_semaforo);

    if(send(kernelSocketClient, buffer, (t_size) buffer_size, 0) < 0) {
        log_error(cpu_log, "wait(%s) failed", identificador_semaforo);
        return ERROR;
    }
    //me quedo esperando activamente a que kernel me responda
    recv(kernelSocketClient, buffer, sizeof(char), 0);
    //kernel_response debería ser 0

    free(buffer);

}

void signal(t_nombre_semaforo identificador_semaforo){
    char* buffer = NULL;
    int buffer_size = sizeof(char) * 2 + 4 + strlen(identificador_semaforo);
    asprintf(&buffer, "%d%d%04d%s", SEMAPHORE_ID, SIGNAL_ID, strlen(identificador_semaforo), identificador_semaforo);

    if(send(kernelSocketClient, buffer, (t_size) buffer_size, 0) < 0) {
        log_error(cpu_log, "signal(%s) failed", identificador_semaforo);
        return ERROR;
    }
}

//En el wait me fijo si el kernel me dice de expropiarme o no, es medio choto porque no es responsabilidad del cpu de esperar estas cosas, sino del kernel.
//En el signal le digo directamente que haga signal y sigo con mi Q, porque no hay expropiacion, sino tendria que expropiarme.
//Para las variables compartidas es un simple recv o send y