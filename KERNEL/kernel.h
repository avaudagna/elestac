#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/log.h>
#include <commons/config.h>
#include <parser/metadata_program.h>
#include "socketCommons.h"

typedef enum {NEW, READY, EXECUTING, BLOCKED, EXIT} enum_queue;

typedef struct {
		int 			pid;
		int 			program_counter;
		int 			stack_pointer;
		t_queue*		stack_index;
		enum_queue 		status;
		t_intructions*	instrucciones_serializado;
		t_size			instrucciones_size;
		t_size			etiquetas_size;
		char*			etiquetas;
	} t_pcb;

typedef struct {
		int 	PUERTO_PROG,
				PUERTO_CPU,
				QUANTUM,
				QUANTUM_SLEEP;
		char** 	SEM_ID;
		char** 	SEM_INIT;
		char** 	IO_ID;
		char** 	IO_SLEEP;
		char** 	SHARED_VARS;
		int 	STACK_SIZE;
		int 	PAGE_SIZE;
		int 	PUERTO_UMC;
		char*	IP_UMC;
		char*	KERNEL_IP;
	} t_setup;

// TODO Delete
typedef struct {
   int pos;
   int cant_args;
   char *args; //12 bytes por arg
   int cant_vars;
   char *vars; //13 bytes por var
   int ret_pos;
   int cant_ret_vars;
   char *ret_vars;// 12 bytes por ret_var
} t_stack_entry;

#define MAX_CLIENTS 100 /* TODO Delete this */

int 	loadConfig(char*);
int 	connect2UMC();
int 	requestPages2UMC(char* PID, int ansisopLen,char* code,int clientUMC);
void 	tratarSeniales(int);


/* Removes client sockets which closed the connection.
 * Returns the ID of the last socket descriptor in the list.
 */
int 	rmvClosedCPUs(int *cpuList, int *cpusOnline);
int 	rmvClosedConsoles(int *consoleList, int *consolesOnline);

void 	killCPU(int cpu);
void 	killCONSOLE(int console);

int 	newClient(int serverSocket, int *clientSocket, int clientsOnline);

t_setup	setup; // GLOBAL settings
char global_buffer_4[4];

#endif