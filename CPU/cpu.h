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

#include <sys/socket.h>
#include <signal.h>

#include "implementation_ansisop.h"

#include "cpu_structs.h"
#include <parser/metadata_program.h>
#include "libs/pcb_tests.h"

#define KERNEL_HANDSHAKE '0'
#define UMC_HANDSHAKE '1'

//cpu state machine states
#define S0_FIRST_COMS 0
#define S1_GET_PCB 1
#define S2_CHANGE_ACTIVE_PROCESS 2
#define S3_EXECUTE 3
#define S4_RETURN_PCB 4

//round robin state machine states
#define S0_CHECK_EXECUTION_STATE 0
#define S1_GET_EXECUTION_LINE 1
#define S2_EXECUTE_LINE 2
#define S3_POSTPROCESS 3

//Cpu-Kernel Messages
#define QUANTUM_END '1'
#define PROGRAM_END '2'

//UMC operations
#define UMC_OK_RESPONSE '1'
#define UMC_HANDSHAKE_RESPONSE_ID '0'
#define PEDIDO_BYTES '3'
#define ALMACENAMIENTO_BYTES '4'
#define FIN_COMUNICACION_CPU '0'

int recibir_pcb(int kernelSocketClient, t_kernel_data *kernel_data_buffer);
t_list * armarDireccionesLogicasList(t_intructions *original_instruction);
void tratarSeniales(int senial);
int loadConfig(char* configFile);
int get_instruction_line(t_list *instruction_addresses_list, void ** instruction_line);
int request_address_data(void ** buffer, logical_addr *address);

//cpu state machine
int cpu_state_machine();
int kernel_first_com();
int umc_first_com();
int get_pcb();
int return_pcb();

//execute process state machine
int execute_state_machine();
int check_execution_state();
int get_execution_line(void ** instruction_line);
int execute_line(void *pVoid);
int change_active_process();
int post_process_operations();

void finished_quantum_post_process();
void send_quantum_end_notif();
void program_end_notification();

//helpers
enum_queue status_check();
void status_update(int anEnd);
void strip_string(char *s);


#endif
