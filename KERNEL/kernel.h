#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
		t_queue			stack_index;
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
	} t_setup;

#define KERNEL_IP "127.0.0.1"
#define IP_UMC "127.0.0.1"  /* can I add this to the config file? */
#define PUERTO_UMC 56793	/* can I add this to the config file? */
#define KERNEL_PORT 54326
#define MAX_CLIENTS 100 /* TODO Delete this */

int 	loadConfig(char*, t_setup*);
int 	connect2UMC();
int 	requestPages2UMC(t_metadata_program* metadata);
void 	tratarSeniales(int);


/* Removes client sockets which closed the connection.
 * Returns the ID of the last socket descriptor in the list.
 */
int 	rmvClosedCPUs(int *cpuList, int *cpusOnline);
int 	rmvClosedConsoles(int *consoleList, int *consolesOnline);

void 	killCPU(int cpu);
void 	killCONSOLE(int console);

int 	newClient(int serverSocket, int *clientSocket, int clientsOnline);

t_setup	setup;

#endif