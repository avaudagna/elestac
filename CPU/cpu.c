
#include "cpu.h"
#include "libs/pcb.h"
#include "libs/stack.h"
#include "cpu_structs.h"

//Variables globales
t_setup * setup; // GLOBAL settings
t_log* cpu_log;
int umcSocketClient = 0, kernelSocketClient = 0;
t_kernel_data * actual_kernel_data;
t_pcb * actual_pcb;
int LAST_QUANTUM_FLAG = 0;

AnSISOP_funciones funciones_generales_ansisop = {
        .AnSISOP_definirVariable		= definirVariable,
        .AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
        .AnSISOP_dereferenciar			= dereferenciar,
        .AnSISOP_asignar				= asignar,
        .AnSISOP_irAlLabel              = irAlLabel,
        .AnSISOP_imprimir				= imprimir,
        .AnSISOP_imprimirTexto			= imprimirTexto,
        .AnSISOP_entradaSalida          = entradaSalida,
        .AnSISOP_llamarConRetorno       = llamarConRetorno,
        .AnSISOP_retornar               = retornar,
        .AnSISOP_llamarSinRetorno       = llamarSinRetorno,
        .AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
        .AnSISOP_asignarValorCompartida = asignarValorCompartida,
};

AnSISOP_kernel funciones_kernel_ansisop = {
        .AnSISOP_signal = la_signal,
        .AnSISOP_wait = la_wait
};


//
//Compilame asi:
// gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu libs/stack.c libs/pcb.c libs/serialize.c libs/socketCommons.c cpu.c implementation_ansisop.c -L/usr/lib -lcommons -lparser-ansisop -lm
//


int main(int argc, char **argv) {

	int i;

	u_int32_t actual_program_counter = 0;// actual_pcb->program_counter;

    if (cpu_init(argc, argv[1])<0) return 0;

    cpu_state_machine();

    log_info(cpu_log, "=== Exiting CPU ===");
    return 0;
}


//Yo voy a recibir los bytes que le vaya pidiendo y los voy metiendo en un buffer hasta que termino
    //Ese buffer representa una linea del programa que tengo que ejecutar,
    //Incremento el PC
    //Mando buffer al parser
    //El parser llama a las primitivas que tengo que tener implementadas
    //Las primitivas son del CPU, no del parser, por lo que voy a tener que tocar el PCB un monton
    // dentro de ellas
    //Si PC = instrucciones_size, el status se cambia a EXIT, esto lo voy chequeando
    //Siempre que hablo con CPU, le serializo y mando el PCB
    //Cuando se cumple el Quantum, cambio el status a Ready
            //Ready termino el quantum pero esta bien
            //Blocked si esta haciendo entrada salida
            //Exit si ya termino


//		if( send(umcSocketClient , "CPU"  , PACKAGE_SIZE , 0) < 0) {
//				puts("Send failed");
//				return 1;
//		}
//		//Wait for response from UMC handshake
//		if( recv(umcSocketClient , umcMessage , PACKAGE_SIZE , 0) <= 0) {
//				puts("recv failed");
//				break;
//		}
//
//		//Send message to the UMC
//		if( send(umcSocketClient , kernelMessage  , PACKAGE_SIZE , 0) < 0) {
//				puts("Send failed");
//				return 1;
//		}
//
//		//Wait for response from UMC
//		if( recv(umcSocketClient , umcMessage , PACKAGE_SIZE , 0) <= 0) {
//				puts("recv failed");
//				break;
//		}
//		puts("UMC reply :");
//		puts(umcMessage);
//
//		log_info(&logFile, "Analizando linea");
//		analizadorLinea(strdup(umcMessage), &functions, &kernel_functions);

		//Aca es donde uso el parser con ese mensaje para generar las lineas a ejecutar
		//de lo que me devuelva el parser, devuelvo el resultado para imprimir al kernel

		//Send message to the Kernel
//		if( send(kernelSocketClient , "Elestac"  , PACKAGE_SIZE , 0) <= 0) {
//				puts("Send failed");
//				return 1;
//		}



int cpu_state_machine() {
    int state = 0;

    while(state != ERROR) {
        switch(state) {
            case S0_FIRST_COMS:
                if ( umc_first_com() == SUCCESS && kernel_first_com() == SUCCESS) { state = S1_GET_PCB; } else { state = ERROR; }
                break;
            case S1_GET_PCB:
                if (get_pcb() == SUCCESS) { state = S2_CHANGE_ACTIVE_PROCESS; } else { state = ERROR; }
                break;
            case S2_CHANGE_ACTIVE_PROCESS:
                if (change_active_process() == SUCCESS) { state = S3_EXECUTE; } else { state = ERROR; }
                break;
            case S3_EXECUTE:
                if (execute_state_machine() == SUCCESS) { state = S4_RETURN_PCB; } else { state = ERROR; }
                break;
            case S4_RETURN_PCB:
                if (return_pcb() == SUCCESS) { state = S1_GET_PCB; } else { state = ERROR; }
                break;
            default:
            case ERROR:
                return ERROR;
                break;
        }
    }
    return SUCCESS;
}

int return_pcb() {
    //Serializo el PCB y lo envio a KERNEL
    void * serialized_pcb = NULL;
    int serialized_buffer_index = 0;
    serialize_pcb(actual_pcb, &serialized_pcb, &serialized_buffer_index);
    testSerializedPCB(actual_pcb, serialized_pcb);
    if( send(kernelSocketClient , &serialized_buffer_index, (int) sizeof(int), 0) < 0) {
        log_error(cpu_log, "Send serialized_buffer_length to KERNEL failed");
        return ERROR;
    }
    log_info(cpu_log, "Pcb Size to send : %d", serialized_buffer_index);
    if( send(kernelSocketClient , serialized_pcb, (int) serialized_buffer_index, 0) < 0) {
        log_error(cpu_log, "Send serialized_pcb to KERNEL failed");
        return ERROR;
    }
    log_info(cpu_log, "Send serialized_pcb to KERNEL was successful");
    return SUCCESS;
}

int execute_state_machine() {
    int execution_state = 0;
    void * instruction_line = NULL;

    while(actual_kernel_data->Q > 0) {
        switch (execution_state) {
            case S0_CHECK_EXECUTION_STATE:
                switch (check_execution_state()) {
                    case SUCCESS:
                        execution_state = S1_GET_EXECUTION_LINE;
                        break;
                    case EXIT:
                        return SUCCESS;
                    case BLOCKED:
                        return BLOCKED;
                    default:
                        execution_state = ERROR;
                        break;
                } break;
            case S1_GET_EXECUTION_LINE:
                if (get_execution_line(&instruction_line) == SUCCESS) {
                    execution_state = S2_EXECUTE_LINE;
                } else {
                    execution_state = ERROR;
                } break;
            case S2_EXECUTE_LINE:
                switch(execute_line(instruction_line)) {
                    case SUCCESS:
                        execution_state = S3_POSTPROCESS;
                        break;
                    case BROKEN:
                    case EXIT:
                        execution_state =  S0_CHECK_EXECUTION_STATE;
                        break;
                    default:
                        execution_state = ERROR;
                } break;
            case S3_POSTPROCESS:
                if(post_process_operations() == SUCCESS) {
                    execution_state = S0_CHECK_EXECUTION_STATE;
                } else {
                    execution_state = ERROR;
                } break;
            default:
            case ERROR:
                return ERROR;
        }
    }
    finished_quantum_post_process();
    return SUCCESS;
}

int post_process_operations() {
    log_info(cpu_log, "Starting delay of : %d seconds", actual_kernel_data->QSleep/1000);
    usleep(actual_kernel_data->QSleep);
    if(LAST_QUANTUM_FLAG) {
        //Signal for exiting CPU was sent
        return EXIT;
    }
    actual_kernel_data->Q--;
    log_info(cpu_log, "Remaining Quantum : %d", actual_kernel_data->Q);
    actual_pcb->program_counter++;
    log_info(cpu_log, "Next execution line :%d", actual_pcb->program_counter);
    return SUCCESS;
}

void finished_quantum_post_process() {
    log_info(cpu_log, "=== Finished Quantum ===");
    if(status_check() == EXECUTING) {
        status_update(READY);
    }
    if(status_check() == BLOCKED) {
        return;
    }
    send_quantum_end_notif();
}

void send_quantum_end_notif() {
    char * operation = calloc(1, sizeof(char));
    *operation = QUANTUM_END;
    if( send(kernelSocketClient , operation, sizeof(char), 0) < 0) {
        log_error(cpu_log, "Send QUANTUM_END to KERNEL failed");
        free(operation);
        return;
    }
    log_info(cpu_log, "Quantum end notification sent to KERNEL");
    free(operation);
}

int execute_line(void *instruction_line) {

    if(strcmp(instruction_line, "end") == 0) {
        //End of program reached
        status_update(EXIT);
        return EXIT;
    }
    analizadorLinea((char*) instruction_line, &funciones_generales_ansisop, &funciones_kernel_ansisop);
    log_info(cpu_log, "Finished line %d execution successfuly", actual_pcb->program_counter);
    return SUCCESS;
}

void status_update(int status) {
    actual_pcb->status = status;
}

int check_execution_state() {
    if(status_check() == EXIT || status_check() == BROKEN){
        program_end_notification();
        return EXIT;
    }
    if(status_check() == BLOCKED) {
        //Just return PCB
        return EXIT;
    }
    return SUCCESS;
}

void program_end_notification() {

    log_info(cpu_log, "=== Finished Program ===");
    log_info(cpu_log, "Finishid executing PCB with PID %d", actual_pcb->pid);
    char operation = PROGRAM_END;
    if( send(kernelSocketClient , &operation, sizeof(char), 0) < 0) {
        log_error(cpu_log, "Send PROGRAM_END to KERNEL failed");
        return;
    }
    log_info(cpu_log, "Program end notification sent to KERNEL");
}


enum_queue status_check() {
    switch (actual_pcb->status) {
        case READY:
            return READY;
        case BLOCKED:
            //check if io has finished
            //and waste Q
//            return BLOCKED;
            return BLOCKED;
        case EXECUTING:
            return EXECUTING;
        case EXIT:
            return EXIT;
        default:
            break;

    }
}

int get_execution_line(void ** instruction_line) {

    t_list * instruction_addresses_list = armarDireccionesLogicasList(
            actual_pcb->instrucciones_serializado + actual_pcb->program_counter);
    get_instruction_line(instruction_addresses_list, instruction_line);
    strip_string(*instruction_line);
    log_info(cpu_log, "Next execution line: %s", *instruction_line);
    return SUCCESS;
}

int umc_first_com() {
    //Send handshake
    char * umc_handshake = calloc(1, sizeof(char));
    *umc_handshake = UMC_HANDSHAKE;
    if( send(umcSocketClient , umc_handshake , sizeof(char), 0) < 0) {
        log_error(cpu_log, "Send UMC handshake %s failed", umc_handshake);
        return ERROR;
    }
    free(umc_handshake);
    //Recv hanshake operation response
    char *operation = calloc(1, sizeof(char));
    if( recv(umcSocketClient , operation, sizeof(char) , 0) <= 0) {
        log_error(cpu_log, "Recv UMC bad operation");
        return ERROR;
    }
    if( *operation != UMC_HANDSHAKE_RESPONSE_ID ) {
        log_error(cpu_log, "Recv UMC bad operation: %c", *(char*)operation);
        return ERROR;
    }
    free(operation);
    //Recv Page Size
    int *umc_buffer = calloc(1, sizeof(int));
    if( recv(umcSocketClient , umc_buffer, sizeof(int) , 0) <= 0) {
        log_error(cpu_log, "Recv UMC PAGE_SIZE failed");
        return ERROR;
    }
    setup->PAGE_SIZE = *umc_buffer;
    free(umc_buffer);
    return SUCCESS;
}

int change_active_process() {
    //send actual process pid
    void * buffer = NULL;
//    asprintf(&buffer, "%d%04d", CAMBIO_PROCESO_ACTIVO, actual_pcb->pid);
    int buffer_index = 0;
    char operacion = CAMBIO_PROCESO_ACTIVO;
    serialize_data(&operacion, sizeof(char), &buffer, &buffer_index);
    serialize_data(&actual_pcb->pid, sizeof(int), &buffer, &buffer_index);
    if( send(umcSocketClient , buffer, (int) buffer_index, 0) < 0) {
        log_error(cpu_log, "Send pid %d to UMC failed", actual_pcb->pid);
        return ERROR;
    }
    free(buffer);
    log_info(cpu_log, "Changed active process to current pid : %d", actual_pcb->pid);
    return SUCCESS;
}

int get_pcb() {
    //Pido por kernel process data
    actual_kernel_data = calloc(1, sizeof(t_kernel_data));
    log_info(cpu_log, "Waiting for kernel PCB and Quantum data");
    if (recibir_pcb(kernelSocketClient, actual_kernel_data) <= 0) {
        log_error(cpu_log, "Error receiving PCB and Quantum data from KERNEL");
        return ERROR;
    }

    //Deserializo el PCB que recibo
    actual_pcb = (t_pcb *) calloc(1,sizeof(t_pcb));
    int  last_buffer_index = 0;
    deserialize_pcb(&actual_pcb, actual_kernel_data->serialized_pcb, &last_buffer_index);

    //Inicializo si tengo que
    if (get_last_entry(actual_pcb->stack_index)== NULL) {
        actual_pcb->stack_index = queue_create();
        t_stack_entry * first_empty_entry = calloc(1, sizeof(t_stack_entry));
        queue_push(actual_pcb->stack_index, first_empty_entry);
    }
    status_update(EXECUTING);
    return SUCCESS;
}

int kernel_first_com() {
    char *kernel_handshake = calloc(1, sizeof(char));
    *kernel_handshake = KERNEL_HANDSHAKE;
    if( send(kernelSocketClient, kernel_handshake, sizeof(char), 0) < 0) {
        log_error(cpu_log, "Error sending handshake to KERNEL");
        return ERROR;
    }
    log_info(cpu_log, "Sent handshake :''%s'' to KERNEL", kernel_handshake);

    return SUCCESS;
}

int get_instruction_line(t_list *instruction_addresses_list, void ** instruction_line) {

    void * recv_bytes_buffer = NULL;
    int buffer_index = 0;
    char response_status;
    while(list_size(instruction_addresses_list) > 0) {

        logical_addr * element = list_get(instruction_addresses_list, 0);

//        sprintf(aux_buffer, "%d%04d%04d%04d", PEDIDO_BYTES, element->page_number, element->offset, element->tamanio);
        void * aux_buffer = NULL;
        int aux_buffer_index = 0;
        char operacion = PEDIDO_BYTES;
        serialize_data(&operacion, sizeof(char), &aux_buffer,&aux_buffer_index);
        serialize_data(&element->page_number, sizeof(int), &aux_buffer, &aux_buffer_index);
        serialize_data(&element->offset, sizeof(int), &aux_buffer, &aux_buffer_index);
        serialize_data(&element->tamanio, sizeof(int), &aux_buffer, &aux_buffer_index);
        log_info(cpu_log, "Fetching for (%d,%d,%d) in UMC", element->page_number, element->offset, element->tamanio);
        //Send bytes request to UMC
        if( send(umcSocketClient, aux_buffer, aux_buffer_index, 0) < 0) {
            puts("Last Fetch failed");
            return ERROR;
        }
        //Recv response
        recv_bytes_buffer = calloc(1, (int) element->tamanio);
        if(recv_bytes_buffer == NULL) {
            log_error(cpu_log, "get_instruction_line recv_bytes_buffer mem alloc failed");
            return ERROR;
        }

        //TODO: Esto lo tengo que hacer en check_umc_response_status y validar que no sea 2 (STACK_OVERFLOW)
        response_status = recv_umc_response_status();
        if(response_status == '1') {
            log_info(cpu_log, "UMC_RESPONSE_OK");
        } else if (response_status == '2') {
            log_error(cpu_log, "=== STACK OVERFLOW ===");
            exit(1);
        }

        if( recv(umcSocketClient , recv_bytes_buffer , (int ) element->tamanio , 0) <= 0) {
            log_error(cpu_log, "UMC bytes recv failed");
            return ERROR;
        }
        log_info(cpu_log, "Bytes Received: %s", (char*) recv_bytes_buffer);

        *instruction_line = realloc(*instruction_line, (int) buffer_index+element->tamanio);

        memcpy(*instruction_line+buffer_index, recv_bytes_buffer, (int) element->tamanio);
        buffer_index += element->tamanio;
        free(recv_bytes_buffer);
        list_remove_and_destroy_element(instruction_addresses_list, 0, free);
    }
    *instruction_line = realloc(*instruction_line, (int) (sizeof(char) + buffer_index));
    * (char*)(*instruction_line+buffer_index) = '\0';
    return SUCCESS;
}


int request_address_data(void ** buffer, logical_addr *address) {
    void * aux_buffer = calloc(1, sizeof(int) * 3 + sizeof(char));
    //TODO: Uncomment if using
//    sprintf(aux_buffer, "%d%04d%04d%04d", atoi(PEDIDO_BYTES), address->page_number, address->offset, address->tamanio);
    log_info(cpu_log, "Fetching for (%d,%d,%d) in UMC", address->page_number, address->offset, address->tamanio);
    printf("\nFetching for (%d,%d,%d) in UMC\n", address->page_number, address->offset, address->tamanio);
    //Send bytes request to UMC
    if( send(umcSocketClient, aux_buffer, strlen(aux_buffer), 0) < 0) {
        puts("Last Fetch failed");
        return ERROR;
    }
    //Recv response
    free(aux_buffer);
    *buffer = calloc(1, (int) address->tamanio);
    if( recv(umcSocketClient , *buffer , (int ) address->tamanio , 0) <= 0) {
        log_error(cpu_log, "UMC bytes recv failed");
        return ERROR;
    }
    log_info(cpu_log, "Bytes Received: %s", *buffer);
    printf("Bytes Received: %s", (char*) *buffer);
    return SUCCESS;
}

int recibir_pcb(int kernelSocketClient, t_kernel_data *kernel_data_buffer) {
    void * buffer = calloc(1,sizeof(int));
    char * kernel_operation = calloc(1, sizeof(char));
    int deserialized_data_index = 0;

    if(buffer == NULL) {
        log_error(cpu_log, "recibir pcb buffer mem alloc failed");
        return ERROR;
    }

    printf(" .:: Waiting PCB from Kernel ::.\n");
    if( recv(kernelSocketClient , kernel_operation , sizeof(char) , 0) <= 0) {
        log_error(cpu_log, "KERNEL handshake response receive failed");
        return ERROR;
    }
    if( *kernel_operation != '1') {
        log_error(cpu_log, "Wrong KERNEL handshake response received");
        return ERROR;
    }
    free(kernel_operation);

    //Quantum data
    if( recv(kernelSocketClient , buffer , sizeof(int) , 0) <= 0) {
        log_error(cpu_log, "Q recv failed");
        return ERROR;
    }
    deserialize_data(&kernel_data_buffer->Q , sizeof(int), buffer, &deserialized_data_index);
    log_info(cpu_log, "Q: %d", kernel_data_buffer->Q);

    if( recv(kernelSocketClient , buffer , sizeof(int) , 0) <= 0) {
        log_error(cpu_log, "QSleep recv failed");
        return ERROR;
    }
    deserialized_data_index = 0;
    deserialize_data(&kernel_data_buffer->QSleep , sizeof(int), buffer, &deserialized_data_index);
    log_info(cpu_log, "QSleep: %d", kernel_data_buffer->QSleep);

    if( recv(kernelSocketClient ,buffer , sizeof(int) , 0) <= 0) {
        log_error(cpu_log, "pcb_size recv failed");
        return ERROR;
    }
    deserialized_data_index = 0;
    deserialize_data(&kernel_data_buffer->pcb_size , sizeof(int), buffer, &deserialized_data_index);
    log_info(cpu_log, "pcb_size: %d", kernel_data_buffer->pcb_size);
    free(buffer);

    kernel_data_buffer->serialized_pcb = calloc(1, (int ) kernel_data_buffer->pcb_size);
    if(kernel_data_buffer->serialized_pcb  == NULL) {
        log_error(cpu_log, "serialized_pcb mem alloc failed");
        return ERROR;
    }
    if( recv(kernelSocketClient , kernel_data_buffer->serialized_pcb , (int )  kernel_data_buffer->pcb_size , 0) <= 0) {
        log_error(cpu_log, "serialized_pcb recv failed");
        return ERROR;
    }

    return SUCCESS;
}

t_list* armarDireccionesLogicasList(t_intructions *original_instruction) {

    t_intructions* actual_instruction = calloc(1, sizeof(t_intructions));
    actual_instruction->start = original_instruction->start;
    actual_instruction->offset = original_instruction->offset;

    t_list *address_list = list_create();
    logical_addr *addr = (logical_addr*) calloc(1,sizeof(logical_addr));
    int actual_page = 0;
    log_info(cpu_log, "Obtaining logic addresses for start:%d offset:%d",actual_instruction->start, actual_instruction->offset );
    //First calculation
    addr->page_number =  (int) actual_instruction->start / setup->PAGE_SIZE;
    actual_page = addr->page_number;
    addr->offset = actual_instruction->start % setup->PAGE_SIZE;
    if(actual_instruction->offset > setup->PAGE_SIZE - addr->offset ) {
        addr->tamanio = setup->PAGE_SIZE - addr->offset;
    } else {
        addr->tamanio = actual_instruction->offset;
    }
    list_add(address_list, addr);
    actual_instruction->offset = actual_instruction->offset - addr->tamanio;

    //ojo que actual_instruction->offset es un int
    //cuando se lo decremente por debajo de 0, asumira su valor maximo
    while (actual_instruction->offset != 0) {
        addr = (logical_addr*) calloc(1,sizeof(logical_addr));
        addr->page_number = ++actual_page;
        addr->offset = 0;
        //addr->tamanio = setup->PAGE_SIZE - addr->offset;
        if(actual_instruction->offset > setup->PAGE_SIZE - addr->offset ) {
            addr->tamanio = setup->PAGE_SIZE - addr->offset;
        } else {
            addr->tamanio = actual_instruction->offset;
        }
        actual_instruction->offset = actual_instruction->offset - addr->tamanio;
        list_add(address_list, addr);
    }
    free(actual_instruction);
    return address_list;
}

void strip_string(char *s) {
    char *s_reference = s;
    while(*s != '\0') {
        if(*s != '\t' && *s != '\n') {
            *s_reference++ = *s++;
        } else {
            ++s;
        }
    }
    *s_reference = '\0';
}
