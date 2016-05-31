#ifndef CPU_H_
#define CPU_H_

//#define UMC_ADDR "127.0.0.1"
//#define UMC_PORT 56793
//#define KERNEL_ADDR "190.105.70.226"
//#define KERNEL_PORT 5001
//#define PAGE_SIZE 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <commons/log.h>
#include <parser/parser.h>
#include "implementation_ansisop.h"

#include "libs/socketCommons.h"


AnSISOP_funciones functions = {
        .AnSISOP_definirVariable		= definirVariable,
        .AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
        .AnSISOP_dereferenciar			= dereferenciar,
        .AnSISOP_asignar				= asignar,
        .AnSISOP_imprimir				= imprimir,
        .AnSISOP_imprimirTexto			= imprimirTexto,

};
AnSISOP_kernel kernel_functions = { };

typedef struct {
    int Q;
    int QSleep;
    size_t pcb_size;
    char *serialized_pcb;
} t_kernel_data;

typedef struct {
    char** 	SEM_ID;
    char** 	SEM_INIT;
    char** 	IO_ID;
    char** 	IO_SLEEP;
    char** 	SHARED_VARS;
    int 	STACK_SIZE;
    int 	PAGE_SIZE;
    int 	PUERTO_UMC;
    char*	IP_UMC;
    int 	PUERTO_KERNEL;
    char*	KERNEL_IP;
} t_setup;

void RecibirPcb(int kernelSocketClient ,t_kernel_data *kernel_data );
#endif
