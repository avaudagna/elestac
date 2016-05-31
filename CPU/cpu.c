#include <parser/metadata_program.h>
#include <commons/config.h>
#include <sys/socket.h>
#include <math.h>
#include <signal.h>
#include "cpu.h"
#include "libs/pcb.h"

void MaquinaDeEstados();

t_setup	setup; // GLOBAL settings
logical_addr * armarDireccionLogica(t_intructions *actual_instruction);
void tratarSeniales(int senial);

//
//Compilame asi:
// gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu libs/stack.c libs/pcb.c libs/serialize.c libs/socketCommons.c cpu.c implementation_ansisop.c -L/usr/lib -lcommons -lparser-ansisop -lm
//
int main(int argc, char **argv) {

	int i;
    char *handshake = (char*) calloc(1, sizeof(char));
	int umcSocketClient = 0, kernelSocketClient = 0;
	char kernelHandShake[] = "Existo";


    //t_kernel_data *kernel_data = (t_kernel_data*) malloc(sizeof(t_kernel_data));
    //t_log logFile = { fopen("cpuLog.log", "r"), true, LOG_LEVEL_INFO, "CPU", 1234 };
	u_int32_t actual_program_counter = 0;// actual_pcb->program_counter;

    if (start_cpu(argc, argv[1])<0) return 0;

    char* kernelMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
	if(kernelMessage == NULL) {
		puts("===Error in kernelMessage malloc===");
		return (-1);
	}
	char* umcMessage = (char*) calloc(sizeof(char),PACKAGE_SIZE);
	if(umcMessage  == NULL) {
		puts("===Error in umcMessage  malloc===");
		return (-1);
	}

	//Crete UMC client
	//TODO: getClientSocket(&umcSocketClient, UMC_ADDR , UMC_PORT);
	//Crete Kernel client
	getClientSocket(&kernelSocketClient, setup.KERNEL_IP, setup.PUERTO_KERNEL);

	//keep communicating with server
	while(1) {
		//Send a handshake to the kernel
        //MaquinaDeEstados();
		if( send(kernelSocketClient , "0" , 1 , 0) < 0) {
				puts("Send failed");
				return 1;
		}
        printf("\nSENT 0 TO KERNEL\n");

		//Wait for PCB from the Kernel

        //primer byte 1 ? => recibirPcb LISTO
        //En Recibir PCB : Cada 4 bytes: Q, QSleep, pcb_size, pcb LISTO
        t_kernel_data *incoming_kernel_data_buffer = calloc(1, sizeof(t_kernel_data));
        do {
            printf(" .:: Waiting for Kernel PCB ::.\n");
            recv(kernelSocketClient, handshake, sizeof(char), 0);
        } while (strncmp(handshake, "1", sizeof(char)) != 0);
        recibirPcb(kernelSocketClient, incoming_kernel_data_buffer);
        //Deserializo el PCB que recibo LISTO
        t_pcb * actual_pcb = (t_pcb *) calloc(1,sizeof(t_pcb));
        deserialize_pcb(&actual_pcb, incoming_kernel_data_buffer->serialized_pcb, &incoming_kernel_data_buffer->pcb_size);
        //Le paso el pid a la umc
//        if( send(umcSocketClient , &actual_pcb->pid , sizeof(actual_pcb->pid ), 0) < 0) {
//            puts("Send failed");
//            return 1;
//        }
        //Vuelvo a mi siguiente linea y hago un for de 0 a Quantum
        for(i = 0; i < incoming_kernel_data_buffer->Q; i++) {
            usleep((u_int32_t ) incoming_kernel_data_buffer->QSleep*1000);
            //Dentro del for leo el PC, con el valor (ej 3)
            logical_addr * intruction_addr = armarDireccionLogica(actual_pcb->instrucciones_serializado+actual_pcb->program_counter);
            //TODO: MANDARLE ESTO A MARIAN!

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
		if( send(kernelSocketClient , "Elestac"  , PACKAGE_SIZE , 0) < 0) {
				puts("Send failed");
				return 1;
		}
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

void recibirPcb(int kernelSocketClient, t_kernel_data *kernel_data_buffer) {

        if( recv(kernelSocketClient , &kernel_data_buffer->Q , sizeof(int) , 0) < 0) {
            puts("Q recv failed\n");
        }
        if( recv(kernelSocketClient , &kernel_data_buffer->QSleep , sizeof(kernel_data_buffer->QSleep) , 0) < 0) {
            puts("QSleep recv failed\n");
        }
        if( recv(kernelSocketClient , &kernel_data_buffer->pcb_size , sizeof(kernel_data_buffer->pcb_size) , 0) < 0) {
            puts("pcb_size recv failed\n");
        }
        kernel_data_buffer->serialized_pcb = calloc(1, kernel_data_buffer->pcb_size);
        if( recv(kernelSocketClient , &kernel_data_buffer->serialized_pcb , kernel_data_buffer->pcb_size , 0) < 0) {
            puts("serialized_pcb recv failed\n");
        }
}

logical_addr * armarDireccionLogica(t_intructions *actual_instruction) {
    logical_addr *addr = (logical_addr*) calloc(1,sizeof(logical_addr));

    float actual_position = ((int) actual_instruction->start + 1)/ setup.PAGE_SIZE;
    addr->page_number = (int) ceilf(actual_position);
    addr->offset = (int) actual_instruction->start - (int) round(actual_position);
    addr->len = actual_instruction->offset;
    return addr;
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
            printf("Esto acabará con el sistema. Presione Ctrl+C una vez más para confirmar.\n\n");
            signal (SIGINT, SIG_DFL); // solo controlo una vez.
            break;
        case SIGPIPE:
            // Trato de escribir en un socket que cerro la conexion.
            printf("La consola o el CPU con el que estabas hablando se murió. Llamá al 911.\n\n");
            signal (SIGPIPE, tratarSeniales);
            break;
        default:
            printf("Otra senial\n");
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