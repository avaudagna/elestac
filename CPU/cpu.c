#include <parser/metadata_program.h>
#include <commons/config.h>
#include <sys/socket.h>
#include <math.h>
#include <signal.h>
#include <commons/collections/list.h>
#include "cpu.h"
#include "libs/pcb.h"
#include "libs/stack.h"
#include "libs/pcb_tests.h"

t_setup	setup; // GLOBAL settings
t_log* cpu_log;

//
//Compilame asi:
// gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu libs/stack.c libs/pcb.c libs/serialize.c libs/socketCommons.c cpu.c implementation_ansisop.c -L/usr/lib -lcommons -lparser-ansisop -lm
//
int loadConfig(char* configFile);
int get_instruction_line(int umcSocketClient, t_list *instruction_addresses_list, void ** instruction_line);

int main(int argc, char **argv) {

	int i;
    char *handshake = (char*) calloc(1, sizeof(char));
	int umcSocketClient = 0, kernelSocketClient = 0;
	char kernelHandShake[2] = "0";


    //t_kernel_data *kernel_data = (t_kernel_data*) malloc(sizeof(t_kernel_data));
    //t_log logFile = { fopen("cpuLog.log", "r"), true, LOG_LEVEL_INFO, "CPU", 1234 };
	u_int32_t actual_program_counter = 0;// actual_pcb->program_counter;

    if (start_cpu(argc, argv[1])<0) return 0;
    cpu_log = log_create("cpu.log", "Elestac-CPU", true, LOG_LEVEL_TRACE);

//    char* kernelMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
//	if(kernelMessage == NULL) {
//		puts("===Error in kernelMessage malloc===");
//		return (-1);
//	}
//	char* umcMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
//	if(umcMessage  == NULL) {
//		puts("===Error in umcMessage  malloc===");
//		return (-1);
//	}

	//Crete UMC client
    if (getClientSocket(&umcSocketClient, setup.IP_UMC , setup.PUERTO_UMC) == 0) {
        log_info(cpu_log,"New UMC socket obtained");
    }
//	//Crete Kernel client
//	if (getClientSocket(&kernelSocketClient, setup.KERNEL_IP, setup.PUERTO_KERNEL) == 0) {
//        log_info(cpu_log,"New KERNEL socket obtained");
//    }
//    else {
//        log_error(cpu_log, "Could not obtain KERNEL socket");
//        return -1;
//    }
	//keep communicating with server
	while(1) {
		//Send a handshake to the kernel
        //MaquinaDeEstados();
//		if( send(kernelSocketClient, kernelHandShake, 1, 0) < 0) {
//            log_error(cpu_log, "Error sending handshake to KERNEL");
//				return 1;
//		}
//        printf("\nSENT 0 TO KERNEL\n");
//        log_info(cpu_log, "Sent handshake :''%s'' to KERNEL", kernelHandShake);

        
//        printf(" .:: Waiting for Kernel handshake response ::.\n");
//        if( recv(kernelSocketClient , handshake , sizeof(char) , 0) < 0) {
//              log_error(cpu_log, "KERNEL handshake response receive failed");
//              return -1;
//        }
//        if(strncmp(handshake, "1", sizeof(char)) != 0) {
//            log_error(cpu_log, "Wrong KERNEL handshake response received");
//            return -1;
//        }

//        t_kernel_data *incoming_kernel_data_buffer = calloc(1, sizeof(t_kernel_data));
//        log_info(cpu_log, "Waiting for kernel PCB and Quantum data");
//        if( recibir_pcb(kernelSocketClient, incoming_kernel_data_buffer) < 0) {
//            log_error(cpu_log, "Error receiving PCB and Quantum data from KERNEL");
//            return -1;
//        }

        //Success!!  
        //Deserializo el PCB que recibo
        t_pcb * actual_pcb = (t_pcb *) calloc(1,sizeof(t_pcb));
        int  last_buffer_index = 0;
//        deserialize_pcb(&actual_pcb, incoming_kernel_data_buffer->serialized_pcb, &last_buffer_index);

        actual_pcb = getPcbExample();
        //Le paso el pid a la umc
        //handshake
        if( send(umcSocketClient , "1" , 1, 0) < 0) {
            puts("Send failed");
            return 1;
        }
        //pid
        if( send(umcSocketClient , &actual_pcb->pid , sizeof(actual_pcb->pid ), 0) < 0) {
            puts("Send failed");
            return 1;
        }

        char identificador_operacion_umc;
        if( recv(umcSocketClient , &identificador_operacion_umc , sizeof(char) , 0) < 0) {
            log_info(cpu_log, "identificador_operacion_umc recv failed");
            return -1;
        }
        //recibo el PAGE_SIZE
        char umc_buffer[4];
        if( recv(umcSocketClient , umc_buffer, sizeof(int) , 0) <= 0) {
            log_info(cpu_log, "serialized_pcb recv failed");
            return -1;
        }
        setup.PAGE_SIZE = atoi(umc_buffer);


        void * instruction_line = NULL;
//        //Voy a la siguiente linea y hago un for de 0 a Quantum
//        for(i = 0; i < incoming_kernel_data_buffer->Q; i++) {
//            usleep((u_int32_t ) incoming_kernel_data_buffer->QSleep*1000);
            t_list * instruction_addresses_list = armarDireccionLogica(actual_pcb->instrucciones_serializado+actual_pcb->program_counter);

            get_instruction_line(umcSocketClient, instruction_addresses_list, &instruction_line);

//

        //}

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
	}

	return 0;
}

int get_instruction_line(int umcSocketClient, t_list *instruction_addresses_list, void ** instruction_line) {

    void * recv_bytes_buffer = NULL;
    int index = 0, buffer_index = 0;

    while(list_size(instruction_addresses_list) > 0) {
        if( send(umcSocketClient , "2", sizeof(char), 0) < 0) {
            puts("Send solicitar bytes de una pagina marker");
            return -1;
        }
        logical_addr * element = list_remove(instruction_addresses_list, index);
        void * buffer = NULL;
        int buffer_size = 0;
        asprintf(&buffer, "%04d%04d%04d", element->page_number, element->offset, element->tamanio);
        if( send(umcSocketClient, buffer, 12, 0) < 0) {
            puts("Send addr instruction_addr");
            return 1;
        }
        index ++;

        recv_bytes_buffer = calloc(1, (size_t) element->tamanio);
        *instruction_line = realloc(*instruction_line, strlen(*instruction_line)+element->tamanio);
        if( recv(umcSocketClient , recv_bytes_buffer , (size_t ) element->tamanio , 0) < 0) {
            log_error(cpu_log, "UMC bytes recv failed");
            return -1;
        }
        log_info(cpu_log, "Bytes Received: %s", recv_bytes_buffer);
        memcpy(*instruction_line+buffer_index, recv_bytes_buffer, (size_t) element->tamanio);
        buffer_index += element->tamanio;
    }
    return 0;
}

void MaquinaDeEstados() {

    //1)

    //2)

    //3)

    //4)

    //5)

    //6)
}

int recibir_pcb(int kernelSocketClient, t_kernel_data *kernel_data_buffer) {
        void * buffer = calloc(1,sizeof(int));
        if( recv(kernelSocketClient , buffer , sizeof(int) , 0) < 0) {
            log_info(cpu_log, "Q recv failed");
            return -1;
        }
        kernel_data_buffer->Q = atoi(buffer);
        log_info(cpu_log, "Q: %d", kernel_data_buffer->Q);


    if( recv(kernelSocketClient ,buffer , sizeof(int) , 0) < 0) {
            log_info(cpu_log, "pcb_size recv failed");
            return -1;
        }
        kernel_data_buffer->pcb_size = atoi(buffer);
        log_info(cpu_log, "pcb_size: %d", kernel_data_buffer->pcb_size);

    kernel_data_buffer->serialized_pcb = calloc(1, (size_t ) kernel_data_buffer->pcb_size);
        if( recv(kernelSocketClient , kernel_data_buffer->serialized_pcb , (size_t )  kernel_data_buffer->pcb_size , 0) < 0) {
            log_info(cpu_log, "serialized_pcb recv failed");
            return -1;
        }
//        char log_buffer[kernel_data_buffer->pcb_size*2];
//        bzero(log_buffer, kernel_data_buffer->pcb_size*2);
//        int i = 0;
//        for (i = 0; i < kernel_data_buffer->pcb_size; i++)
//        {
//            asprintf(&log_buffer, "%02X", *(char*)(kernel_data_buffer->serialized_pcb+i));
//        }
//        log_info(cpu_log, "serialized_pcb: %s", log_buffer);

        return 0;
}

t_list* armarDireccionLogica(t_intructions *actual_instruction) {


//    float actual_position = ((int) actual_instruction->start + 1)/ setup.PAGE_SIZE;
//    addr->page_number = (int) ceilf(actual_position);
//    addr->offset = (int) actual_instruction->start - (int) round(actual_position);
//    addr->tamanio = actual_instruction->offset;
//    return addr;
    t_list *address_list = list_create();
    logical_addr *addr = (logical_addr*) calloc(1,sizeof(logical_addr));
    int actual_page = 0;
    //First calculation
    addr->page_number =  (int) actual_instruction->start / setup.PAGE_SIZE;
    actual_page = addr->page_number;
    addr->offset = actual_instruction->start % setup.PAGE_SIZE;
    if(actual_instruction->offset > setup.PAGE_SIZE) {
        addr->tamanio= setup.PAGE_SIZE;
    }
    else {
        addr->tamanio = actual_instruction->offset;
    }
    list_add(address_list, addr);
    actual_instruction->offset = actual_instruction->offset - addr->tamanio;

    while (actual_instruction->offset > 0) {
        addr = (logical_addr*) calloc(1,sizeof(logical_addr));
        addr->page_number = ++actual_page;
        addr->offset = 0;
        if(actual_instruction->offset > setup.PAGE_SIZE) {
            addr->tamanio = setup.PAGE_SIZE;
        } else {
            addr->tamanio = actual_instruction->offset;
        }
        actual_instruction->offset = actual_instruction->offset - addr->tamanio;
        list_add(address_list, addr);
    }
    return address_list;
}

int start_cpu(int argc, char* configFile){
    printf("\n\t===========================================================\n");
    printf("\t.:: Me llama usted, entonces voy, el CPU es quien yo soy ::.");
    printf("\n\t===========================================================\n\n");
    if(argc==2){
        if(loadConfig(configFile)<0){
            puts(" Config file can not be loaded.\n Please, try again.\n");
            return -1;
        }
    }else{
        printf(" Usage: ./cpu setup.data \n");
        return -1;
    }
    signal (SIGINT, tratarSeniales);
    signal (SIGPIPE, tratarSeniales);
    return 0;
}

int loadConfig(char* configFile){
    if(configFile == NULL){
        return -1;
    }
    t_config *config = config_create(configFile);
    printf(" .:: Loading settings ::.\n");

    if(config != NULL){
        setup.PUERTO_KERNEL=config_get_int_value(config,"PUERTO_KERNEL");
        setup.KERNEL_IP=config_get_string_value(config,"KERNEL_IP");
        setup.IO_ID=config_get_array_value(config,"IO_ID");
        setup.IO_SLEEP=config_get_array_value(config,"IO_SLEEP");
        setup.SEM_ID=config_get_array_value(config,"SEM_ID");
        setup.SEM_INIT=config_get_array_value(config,"SEM_INIT");
        setup.SHARED_VARS=config_get_array_value(config,"SHARED_VARS");
        setup.STACK_SIZE=config_get_int_value(config,"STACK_SIZE");
        setup.PUERTO_UMC=config_get_int_value(config,"PUERTO_UMC");
        setup.IP_UMC=config_get_string_value(config,"IP_UMC");
    }
    //config_destroy(config);
    return 0;
}

void tratarSeniales(int senial){
    printf("\n\t=============================================\n");
    printf("\t\tSystem received the signal: %d",senial);
    printf("\n\t=============================================\n");
    switch (senial){
        case SIGINT:
            // Detecta Ctrl+C y evita el cierre.
            printf("This will end the CPU. Press Ctrl+C again to confirm.\n\n");
            signal (SIGINT, SIG_DFL); // solo controlo una vez.
            break;
        case SIGPIPE:
            // Trato de escribir en un socket que cerro la conexion.
            printf("The KERNEL or UMC connection droped down.\n\n");
            signal (SIGPIPE, tratarSeniales);
            break;
        default:
            printf("Other signal received\n");
            break;
    }
}
/*
void obtener_linea (char *instrucciones_serializado, int index) {

    t_intructions *instruccion_actual= NULL;
    char * direcciones_logicas = NULL;
    int cantidad_direcciones = 0;
    instruccion_actual = obtener_instruccion(instrucciones_serializado, index);
    cantidad_direcciones = obtener_direcciones_logicas(instruccion_actual, direcciones_logicas);
    obtener_linea(instruccion_actual); // la linea que tengo que mandarle al parser para que ejecute


}

void obtener_linea() {
    //Le pido a la UMC la data
}

t_intructions* obtener_instruccion (char *instrucciones_serializado, int index) {
    char *buffer = malloc(sizeof(t_intructions));
    t_intructions* instruccion_auxiliar = (t_intructions*) malloc(sizeof(t_intructions));
    memcpy(buffer, instrucciones_serializado+index*sizeof(t_intructions), sizeof(t_intructions));
    memcpy(instruccion_auxiliar->start, buffer, sizeof(instruccion_auxiliar->start));
    memcpy(instruccion_auxiliar->offset, buffer+sizeof(instruccion_auxiliar->start), sizeof(instruccion_auxiliar->offset));
    return instruccion_auxiliar;
}


void obtener_direcciones_logicas(t_intructions *instruccion_actual, char* direcciones_logicas){
    int cantidad_direcciones = 0;
    char *direccion_logica = NULL;
    calcular_direccion_logica(instruccion_actual);

}

void calcular_direccion_logica(t_intructions *instruccion_actual) {
    int diferencia = (int) instruccion_actual->offset - (int) instruccion_actual->start;
    if(diferencia > PAGE_SIZE) {

    }
}*/