#ifndef KERNEL_H_
#define KERNEL_H_

typedef enum {NEW, READY, EXECUTING, BLOCKED, EXIT} enum_queue;

typedef struct {
		uint32_t pid;
		uint32_t program_counter;
		uint32_t stack_pointer;
		t_list stack_index;
		enum_queue status;
		t_intructions instrucciones_serializado;
		t_size instrucciones_size;
	} t_pcb;

typedef struct {
		int 	PUERTO_PROG,
				PUERTO_CPU,
				QUANTUM,
				QUANTUM_SLEEP;
		char** 	SEM_IDS;
		int* 	SEM_INIT;
		char** 	IO_IDS;
		int* 	IO_SLEEP;
		char** 	SHARED_VARS;
		int 	STACK_SIZE;
	} t_setup;

#define MAX_CLIENTS 100
#define KERNEL_IP "127.0.0.1"
#define LOCALHOST "127.0.0.1"
#define IP_UMC "127.0.0.1"
#define PUERTO_UMC 56793
#define PROTOCOL_SIZE 2
#define KERNEL_PORT 54326
#define STACK_SIZE 2

#endif