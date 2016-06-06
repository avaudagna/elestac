#include <punycode.h>
#include "cpu.h"


//Variables globales
t_setup * setup; // GLOBAL settings
t_log* cpu_log;
int umcSocketClient = 0, kernelSocketClient = 0;
t_kernel_data * actual_kernel_data;
t_pcb * actual_pcb;

AnSISOP_funciones funciones_generales_ansisop = {
        .AnSISOP_definirVariable		= definirVariable,
        .AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
        .AnSISOP_dereferenciar			= dereferenciar,
        .AnSISOP_asignar				= asignar,
        .AnSISOP_imprimir				= imprimir,
        .AnSISOP_imprimirTexto			= imprimirTexto,

};
AnSISOP_kernel funciones_kernel_ansisop = { };


//
//Compilame asi:
// gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu libs/stack.c libs/pcb.c libs/serialize.c libs/socketCommons.c cpu.c implementation_ansisop.c -L/usr/lib -lcommons -lparser-ansisop -lm
//

int main(int argc, char **argv) {

	int i;

	u_int32_t actual_program_counter = 0;// actual_pcb->program_counter;

    if (cpu_init(argc, argv[1])<0) return 0;

    cpu_state_machine();

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
//		if( recv(umcSocketClient , umcMessage , PACKAGE_SIZE , 0) < 0) {
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
//		if( recv(umcSocketClient , umcMessage , PACKAGE_SIZE , 0) < 0) {
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
//		if( send(kernelSocketClient , "Elestac"  , PACKAGE_SIZE , 0) < 0) {
//				puts("Send failed");
//				return 1;
//		}



int cpu_state_machine() {
    int state = 0;

    while(state != ERROR) {
        switch(state) {
            case S0_KERNEL_FIRST_COM:
                if (kernel_first_com() == SUCCESS) { state = S1_GET_PCB; } else { state = ERROR; };
                break;
            case S1_GET_PCB:
                if (get_pcb() == SUCCESS) { state = S2_GET_PAGE_SIZE; } else { state = ERROR; };
                break;
            case S2_GET_PAGE_SIZE:
                if (get_page_size() == SUCCESS) { state = S3_EXECUTE; } else { state = ERROR; };
                break;
            case S3_EXECUTE:
                if (execute_state_machine() == SUCCESS) { state = S4_RETURN_PCB; } else { state = ERROR; };
                break;
            default:
            case ERROR:
                return ERROR;
                break;
        }
    }
    return SUCCESS;
}

int execute_state_machine() {
    int execution_state = 0;
    void * instruction_line = NULL;

    while(actual_kernel_data->Q > 0)
    switch(execution_state) {
        case S0_CHECK_EXECUTION_STATE:
            check_execution_state();
            break;
        case S1_GET_EXECUTION_LINE:
            if (get_execution_line(&instruction_line) == SUCCESS) { execution_state = S2_EXECUTE_LINE; } else { execution_state = ERROR; };
            break;
        case S2_EXECUTE_LINE:
            if (execute_line(instruction_line) == SUCCESS) { execution_state = S3_DECREMENT_Q; } else { execution_state = ERROR; };
            break;
        case S3_DECREMENT_Q:
            actual_kernel_data->Q--;
            break;
        default:
        case ERROR:
            return ERROR;
    }
}

int execute_line(void *instruction_line) {
    analizadorLinea((char*) instruction_line, &funciones_generales_ansisop, &funciones_kernel_ansisop);
    return SUCCESS;
}

int check_execution_state() {
    //TODO: Also check for IO and blocked state
//    if(condicion de corte) {
//        status FIN_DE_EJECUCION
//    }
    actual_pcb->status = EXECUTING;
    return SUCCESS;
}

int get_execution_line(void ** instruction_line) {

    t_list * instruction_addresses_list = armarDireccionLogica(actual_pcb->instrucciones_serializado+actual_pcb->program_counter);
    get_instruction_line(umcSocketClient, instruction_addresses_list, instruction_line);
    return SUCCESS;
}

int get_page_size() {
    //send handshake
    if( send(umcSocketClient , UMC_HANDSHAKE , 1, 0) < 0) {
        log_error(cpu_log, "Send UMC handshake %s failed", UMC_HANDSHAKE);
        return ERROR;
    }
    //send actual process pid
    if( send(umcSocketClient , &actual_pcb->pid , sizeof(actual_pcb->pid ), 0) < 0) {
        log_error(cpu_log, "Send pid %d to UMC failed", actual_pcb->pid);
        return ERROR;
    }
    //recv operation tag
    char identificador_operacion_umc;
    if( recv(umcSocketClient , &identificador_operacion_umc , sizeof(char) , 0) < 0) {
        log_error(cpu_log, "Recv UMC operation identifier failed");
        return ERROR;
    }
    //recv page size
    char umc_buffer[4];
    if( recv(umcSocketClient , umc_buffer, sizeof(int) , 0) <= 0) {
        log_error(cpu_log, "Recv UMC PAGE_SIZE failed");
        return ERROR;
    }
    setup->PAGE_SIZE = atoi(umc_buffer);
    return SUCCESS;
}

int get_pcb() {
    //Pido por kernel process data
    actual_kernel_data = calloc(1, sizeof(t_kernel_data));
    log_info(cpu_log, "Waiting for kernel PCB and Quantum data");
    if( recibir_pcb(kernelSocketClient, actual_kernel_data) < 0) {
        log_error(cpu_log, "Error receiving PCB and Quantum data from KERNEL");
        return ERROR;
    }

    //Success!!
    //Deserializo el PCB que recibo
    actual_pcb = (t_pcb *) calloc(1,sizeof(t_pcb));
    int  last_buffer_index = 0;
    deserialize_pcb(&actual_pcb, actual_kernel_data->serialized_pcb, &last_buffer_index);
    return SUCCESS;
}

int kernel_first_com() {
    char kernel_handshake[2] = KERNEL_HANDSHAKE;
    char kernel_operation [2] = "";

    if( send(kernelSocketClient, kernel_handshake, 1, 0) < 0) {
        log_error(cpu_log, "Error sending handshake to KERNEL");
        return ERROR;
    }
    log_info(cpu_log, "Sent handshake :''%s'' to KERNEL", kernel_handshake);

    printf(" .:: Waiting for Kernel handshake response ::.\n");
    if( recv(kernelSocketClient , kernel_operation , sizeof(char) , 0) < 0) {
        log_error(cpu_log, "KERNEL handshake response receive failed");
        return ERROR;
    }
    if(strncmp(kernel_operation, "1", sizeof(char)) != 0) {
        log_error(cpu_log, "Wrong KERNEL handshake response received");
        return ERROR;
    }
    return SUCCESS;
}

int get_instruction_line(int umcSocketClient, t_list *instruction_addresses_list, void ** instruction_line) {

    void * recv_bytes_buffer = NULL;
    int index = 0, buffer_index = 0;

    while(list_size(instruction_addresses_list) > 0) {
        if( send(umcSocketClient , "2", sizeof(char), 0) < 0) {
            puts("Send solicitar bytes de una pagina marker");
            return ERROR;
        }
        logical_addr * element = list_remove(instruction_addresses_list, 0);
        void * buffer = NULL;
        int buffer_size = 0;
        asprintf(&buffer, "%04d%04d%04d", element->page_number, element->offset, element->tamanio);
        log_info(cpu_log, "Fetching for (%d,%d,%d) in UMC", element->page_number, element->offset, element->tamanio);
        if( send(umcSocketClient, buffer, 12, 0) < 0) {
            puts("Send addr instruction_addr");
            return ERROR;
        }

        recv_bytes_buffer = calloc(1, (size_t) element->tamanio);
        if( recv(umcSocketClient , recv_bytes_buffer , (size_t ) element->tamanio , 0) < 0) {
            log_error(cpu_log, "UMC bytes recv failed");
            return ERROR;
        }
        log_info(cpu_log, "Bytes Received: %s", recv_bytes_buffer);

        *instruction_line = realloc(*instruction_line, (size_t) buffer_index +element->tamanio);
        memcpy(*instruction_line+buffer_index, recv_bytes_buffer, (size_t) element->tamanio);
        buffer_index += element->tamanio;
        free(recv_bytes_buffer);
    }
    return SUCCESS;
}

int recibir_pcb(int kernelSocketClient, t_kernel_data *kernel_data_buffer) {
    void * buffer = calloc(1,sizeof(int));
    if( recv(kernelSocketClient , buffer , sizeof(int) , 0) < 0) {
        log_error(cpu_log, "Q recv failed");
        return ERROR;
    }
    kernel_data_buffer->Q = atoi(buffer);
    log_info(cpu_log, "Q: %d", kernel_data_buffer->Q);

    if( recv(kernelSocketClient , buffer , sizeof(int) , 0) < 0) {
        log_error(cpu_log, "QSleep recv failed");
        return ERROR;
    }
    kernel_data_buffer->QSleep = atoi(buffer);
    log_info(cpu_log, "QSleep: %d", kernel_data_buffer->QSleep);

    if( recv(kernelSocketClient ,buffer , sizeof(int) , 0) < 0) {
        log_error(cpu_log, "pcb_size recv failed");
        return ERROR;
    }

    kernel_data_buffer->pcb_size = atoi(buffer);
    log_info(cpu_log, "pcb_size: %d", kernel_data_buffer->pcb_size);

    kernel_data_buffer->serialized_pcb = calloc(1, (size_t ) kernel_data_buffer->pcb_size);
    if( recv(kernelSocketClient , kernel_data_buffer->serialized_pcb , (size_t )  kernel_data_buffer->pcb_size , 0) < 0) {
        log_error(cpu_log, "serialized_pcb recv failed");
        return ERROR;
    }

    return SUCCESS;
}

t_list* armarDireccionLogica(t_intructions *actual_instruction) {

//    float actual_position = ((int) actual_instruction->start + 1)/ setup->PAGE_SIZE;
//    addr->page_number = (int) ceilf(actual_position);
//    addr->offset = (int) actual_instruction->start - (int) round(actual_position);
//    addr->tamanio = actual_instruction->offset;
//    return addr;
    t_list *address_list = list_create();
    logical_addr *addr = (logical_addr*) calloc(1,sizeof(logical_addr));
    int actual_page = 0;
    log_info(cpu_log, "Obtaining logic addresses for start:%d offset:%d",actual_instruction->start, actual_instruction->offset );
    //First calculation
    addr->page_number =  (int) actual_instruction->start / setup->PAGE_SIZE;
    actual_page = addr->page_number;
    addr->offset = actual_instruction->start % setup->PAGE_SIZE;
    addr->tamanio = setup->PAGE_SIZE - addr->offset;

    list_add(address_list, addr);
    actual_instruction->offset = actual_instruction->offset - addr->tamanio;

    //ojo que actual_instruction->offset es un size_t
    //cuando se lo decremente por debajo de 0, asumira su valor maximo
    while (actual_instruction->offset != 0) {
        addr = (logical_addr*) calloc(1,sizeof(logical_addr));
        addr->page_number = ++actual_page;
        addr->offset = 0;
        //addr->tamanio = setup->PAGE_SIZE - addr->offset;
        if(actual_instruction->offset > setup->PAGE_SIZE) {
            addr->tamanio = setup->PAGE_SIZE;
        } else {
            addr->tamanio = actual_instruction->offset;
        }
        actual_instruction->offset = actual_instruction->offset - addr->tamanio;
        list_add(address_list, addr);
    }
    return address_list;
}

