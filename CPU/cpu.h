#ifndef CPU_H_
#define CPU_H_

#include "cpu_init.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>

#include "libs/socketCommons.h"
#include "libs/stack.h"
#include "libs/t_setup.h"

#include <sys/socket.h>
#include <signal.h>

#include "implementation_ansisop.h"

#include "cpu_structs.h"

#define KERNEL_HANDSHAKE "0"
#define UMC_HANDSHAKE "1"

//cpu state machine states
#define S0_KERNEL_FIRST_COM 0
#define S1_GET_PCB 1
#define S2_GET_PAGE_SIZE 2
#define S3_EXECUTE 3
#define S4_RETURN_PCB 4

//round robin state machine states
#define S0_CHECK_EXECUTION_STATE 0
#define S1_GET_EXECUTION_LINE 1
#define S2_EXECUTE_LINE 2
#define S3_DECREMENT_Q 3

int recibir_pcb(int kernelSocketClient, t_kernel_data *kernel_data_buffer);
t_list * armarDireccionLogica(t_intructions *actual_instruction);
void tratarSeniales(int senial);
int loadConfig(char* configFile);
int get_instruction_line(int umcSocketClient, t_list *instruction_addresses_list, void ** instruction_line);

//cpu state machine
int cpu_state_machine();
int kernel_first_com();
int get_pcb();
int get_page_size();

//execute process state machine
int execute_state_machine();
int check_execution_state();
int get_execution_line(void ** instruction_line);
int execute_line(void *pVoid);

#endif
