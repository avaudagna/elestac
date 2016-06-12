#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <parser/metadata_program.h>
#include "socketCommons.h"
#include "libs/serialize.h"
#include "libs/pcb.h"
#include "libs/stack.h"

struct {
		int 	PUERTO_PROG,
				PUERTO_CPU,
				QUANTUM,
				QUANTUM_SLEEP;
		char** 	SEM_ID;
		char** 	SEM_INIT;
		char** 	IO_ID;
		char** 	IO_SLEEP;
		int     IO_COUNT;
		char** 	SHARED_VARS;
		int*    SHARED_VALUES;
		int 	STACK_SIZE;
		int 	PAGE_SIZE;
		int 	PUERTO_UMC;
		char*	IP_UMC;
		char*	KERNEL_IP;
	} setup;

typedef struct {
	int 	clientID,
			status,
			pid;
} t_Client;

typedef struct {
	int		pid,
			io_index;
	char    *io_name,
			*io_units;
} t_io;

int     requestPages2UMC(char* PID, int ansisopLen,char* code,int clientUMC);
int		start_kernel(int argc, char* configFile);
int 	loadConfig(char* configFile);
int 	connect2UMC();
int		control_clients();
int 	accept_new_client(char* what,int *server, fd_set *sockets,t_list *lista);
int     getIOindex(char *io_name);
int     getSharedIndex(char *shared_name);
void    call_handlers();
void	accept_new_PCB(int newConsole);
void 	tratarSeniales(int);
void 	round_robin();
void	add2FD_SET(void *client);
void 	check_CPU_FD_ISSET(void *client);
void	check_CONSOLE_FD_ISSET(void *client);
void    end_program(int pid, bool consoleStillOpen);
void    *do_work(void *p);
pthread_mutex_t mut_io_list;
sem_t *semaforo_io;
#endif /* KERNEL_H_ */