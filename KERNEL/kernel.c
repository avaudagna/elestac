/* Kernel.c by pacevedo */
/* Compilame asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o kernel socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c kernel.c -L/usr/lib -lcommons -lparser-ansisop
 * El CPU compilalo asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu implementation_ansisop.c libs/socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c cpu.c -L/usr/lib -lcommons -lparser-ansisop -lm
 * Y la consola asi:
gcc -o test_pablo_console socketCommons/socketCommons.c test_pablo_console.c
*/
#include "kernel.h"

/* BEGIN OF GLOBAL STUFF I NEED EVERYWHERE */
int consoleServer, cpuServer, clientUMC;
int maxSocket=0;
t_list	*PCB_READY, *PCB_BLOCKED, *PCB_EXIT; /* I think I don't need queues 4 NEW & EXECUTING */
t_list *cpus_conectadas, *consolas_conectadas;
fd_set 	allSockets;
t_log* kernel_log;
/* END OF GLOBAL STUFF I NEED EVERYWHERE */

int main (int argc, char **argv){
	kernel_log = log_create("kernel.log", "Elestac-KERNEL", true, LOG_LEVEL_TRACE);
	if (start_kernel(argc, argv[1])<0) return 0; //load settings
	clientUMC=connect2UMC();
	//clientUMC=100;setup.PAGE_SIZE=1024; //TODO Delete -> lo hace connect2UMC()
	if (clientUMC<0){
		log_error(kernel_log, "Could not connect to the UMC. Please, try again.");
		return 0;
	}
	PCB_READY = list_create();
	cpus_conectadas = list_create();
	consolas_conectadas= list_create();
	if(setServerSocket(&consoleServer, setup.KERNEL_IP, setup.PUERTO_PROG)<0){
		log_error(kernel_log,"Error while creating the CONSOLE server.");
		return 0;
	}
	if(setServerSocket(&cpuServer, setup.KERNEL_IP, setup.PUERTO_CPU)<0){
		log_error(kernel_log,"Error while creating the CPU server.");
		return 0;
	}
	maxSocket=cpuServer;
	printf(" .:: Servers to CPUs and consoles up and running ::.\n");
	printf(" .:: Waiting for incoming connections ::.\n\n");
	while (control_clients());
	log_error(kernel_log, "Closing kernel");
	close(consoleServer);
	close(cpuServer);
	close(clientUMC); // TODO un-comment when real UMC is present
	log_destroy(kernel_log);
	return 0;
}

int start_kernel(int argc, char* configFile){
	printf("\n\t===========================================\n");
	printf("\t.:: Vamo a calmarno que viene el Kernel ::.");
	printf("\n\t===========================================\n\n");
	if (argc==2){
		if (loadConfig(configFile)<0){
    		puts(" Config file can not be loaded.\n Please, try again.\n");
			log_error(kernel_log, "Config file can not be loaded. Please, try again.");
    		return -1;
    	}
	} else {
		int i = 0;
		for (i = 0; i < 10001; i++){
			usleep(400);
			printf("\r\e[?25l Loading... %d", i/100);
		}
    	printf("\r Usage: ./kernel setup.data \n");
		log_error(kernel_log, "Config file was not provided.");
    	return -1;
	}
	signal (SIGINT, tratarSeniales);
	signal (SIGPIPE, tratarSeniales);
	return 0;
}

int loadConfig(char* configFile){
	if (configFile == NULL)	return -1;
	t_config *config = config_create(configFile);
	printf(" .:: Loading settings ::.\n");
	if (config != NULL){
		setup.PUERTO_PROG=config_get_int_value(config,"PUERTO_PROG");
		setup.PUERTO_CPU=config_get_int_value(config,"PUERTO_CPU");
		setup.QUANTUM=config_get_int_value(config,"QUANTUM");
		setup.QUANTUM_SLEEP=config_get_int_value(config,"QUANTUM_SLEEP");
		setup.IO_ID=config_get_array_value(config,"IO_ID");
		setup.IO_SLEEP=config_get_array_value(config,"IO_SLEEP");
		setup.SEM_ID=config_get_array_value(config,"SEM_ID");
		setup.SEM_INIT=config_get_array_value(config,"SEM_INIT");
		setup.SHARED_VARS=config_get_array_value(config,"SHARED_VARS");
		setup.STACK_SIZE=config_get_int_value(config,"STACK_SIZE");
		setup.PUERTO_UMC=config_get_int_value(config,"PUERTO_UMC");
		setup.IP_UMC=config_get_string_value(config,"IP_UMC");
		setup.KERNEL_IP=config_get_string_value(config,"KERNEL_IP");
	}
	//config_destroy(config);
	return 0;
}

int connect2UMC(){
	int clientUMC;
	char* buffer=malloc(5);
	char* buffer_4=malloc(4);
	printf(" .:: Connecting to UMC on %s:%d ::.\n",setup.IP_UMC,setup.PUERTO_UMC);
	if (getClientSocket(&clientUMC, setup.IP_UMC, setup.PUERTO_UMC)) return (-1);
	sprintf(buffer_4, "%04d", setup.STACK_SIZE);
	asprintf(&buffer, "%s%s", "0", buffer_4);
	send(clientUMC, buffer, 5 , 0);
	printf(" .:: Stack size (sent to UMC): %s ::.\n",buffer_4);
	if (recv(clientUMC, buffer_4, 4, 0) < 0) return (-1);
	setup.PAGE_SIZE=atoi(buffer_4);
	printf(" .:: Page size: (received from UMC): %d ::.\n",setup.PAGE_SIZE);
	free(buffer);
	free(buffer_4);
	return clientUMC;
}

uint32_t requestPages2UMC(char* PID, size_t ansisopLen,char* code,int clientUMC){
	/*
	This function MUST be in a thread
	Because the recv() is BLOCKER and it can be delayed when waiting
	for SWAP.
	Put all parameters in a serialized struct (create a stream);
	*/
	char* buffer;
	char buffer_4[4];
	size_t bufferLen=1+4+4+4+ansisopLen; //1+PID+req_pages+size+code
	sprintf(buffer_4, "%04d", (int) (ansisopLen/setup.PAGE_SIZE)+1);
	asprintf(&buffer, "%d%s%s%04d%s", 1,PID,buffer_4, ansisopLen,code);
	send(clientUMC, buffer, bufferLen, 0);
	recv(clientUMC, buffer_4, 4, 0);
	uint32_t code_pages = (uint32_t) atoi(buffer_4);
	free(buffer);
	return code_pages;
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

int killClient(int client,char *what){
	close(client);
	printf("Bye bye %s!\n", what);
	return 0;
}

void add2FD_SET(void *client){
	t_Client *cliente=client;
	FD_SET(cliente->clientID, &allSockets);
}

void check_CPU_FD_ISSET(void *cpu){
	char cpu_protocol[1];
	t_Client *laCPU = cpu;
	size_t pcb_size;
	void *pcb_serializado;
	char tmp_buff[4];
	t_pcb *incomingPCB;
	if (FD_ISSET(laCPU->clientID, &allSockets)) {
		if (recv(laCPU->clientID, cpu_protocol, 1, 0) > 0){
			log_info(kernel_log,"CPU dijo: %s - Ejecutar protocolo correspondiente",cpu_protocol);
			switch (atoi(cpu_protocol)){
			case 1: // Quantum finish
			case 2: // Program END
				if(recv(laCPU->clientID, tmp_buff, 4, 0) > 0){
					pcb_size=atoi(tmp_buff);
					if(recv(laCPU->clientID, pcb_serializado, pcb_size, 0) > 0){
						deserialize_pcb(&incomingPCB,&pcb_serializado,&pcb_size);
						switch(incomingPCB->status){
							case READY:
								list_add(PCB_READY,incomingPCB);
								break;
							case BLOCKED:
								list_add(PCB_BLOCKED,incomingPCB);
								break;
							case EXIT:
								list_add(PCB_EXIT,incomingPCB);
								break;
						}
					}
				}
				break;
			case 3:				//3== IO
				break;
			case 4:				//4== semaforo
				break;
			case 5:				//5== var compartida
				break;
			default:
				log_error(kernel_log,"Caso no contemplado. CPU dijo: %s",cpu_protocol);
			}
			laCPU->status=READY;
			laCPU->pid=0;
			list_add(cpus_conectadas, laCPU); /* return the CPU to the queue */
			round_robin(); // aca hago algo con los PCB_READY
			/* agregar aca 2 rutinas, una para los PCB_BLOCKED y otra para los PCB_EXIT */
		} else {
			printf(" .:: CPU %d has closed the connection ::. \n", laCPU->clientID);
			killClient(laCPU->clientID,"CPU");
			bool getCPUIndex(void *nbr){
				t_Client *unCliente = nbr;
				return laCPU->clientID == unCliente->clientID;
			}
			list_remove_by_condition(cpus_conectadas, getCPUIndex);
		}
	}
}

void destroy_PCB(void *pcb){
	t_pcb *unPCB = pcb;
	free(unPCB); // TODO check if this will actually CLEAN the PCB;
}

void check_CONSOLE_FD_ISSET(void *console){
	char buffer_4[4];
	t_Client *cliente = console;
	if (FD_ISSET(cliente->clientID, &allSockets)) {
		if (recv(cliente->clientID, buffer_4, 2, 0) > 0){
			log_error(kernel_log,"Consola no deberia enviar nada pero dijo: %s",buffer_4);
		} else {
			char PID[4];
			sprintf(PID, "%04d", (int) cliente->clientID);
			printf(" .:: A console has closed the connection, the associated PID %s will be terminated ::. \n", PID);
			/* Delete the PCB */
			bool match_PCB(void *pcb){
				t_pcb *unPCB = pcb;
				bool matchea = (atoi(PID)==unPCB->pid); // TODO check if this fnc matches the right PCB
				if (matchea && unPCB->status==2){
					// if pcb is being held by CPU -> wait ! -> add it to the PCB_EXIT list
				}
			}
			list_remove_and_destroy_by_condition(PCB_READY,match_PCB,destroy_PCB); // TODO check if this will actually remove the PCB
			killClient(cliente->clientID,"console");
			bool getConsoleIndex(void *nbr){
				t_Client *unCliente = nbr;
				return cliente->clientID == unCliente->clientID;
			}
			//list_remove_by_condition(consolas_conectadas, getConsoleIndex); // TODO Fix this
		}
	}
}

int control_clients(){
	int i, newConsole,newCPU;
	struct timeval timeout = {.tv_sec = 3};
	FD_ZERO(&allSockets);
	FD_SET(cpuServer, &allSockets);
	FD_SET(consoleServer, &allSockets);
	list_iterate(consolas_conectadas,add2FD_SET);
	list_iterate(cpus_conectadas,add2FD_SET);
	int retval=select(maxSocket+1, &allSockets, NULL, NULL, &timeout); // (retval < 0) <=> signal
	if (retval>0) {
		//printf("Tengo %d consolas\n", list_size(consolas_conectadas));
		list_iterate(consolas_conectadas,check_CONSOLE_FD_ISSET);
		list_iterate(cpus_conectadas,check_CPU_FD_ISSET);
		if ((newConsole=accept_new_client("console", &consoleServer, &allSockets, consolas_conectadas)) > 1)
			accept_new_PCB(newConsole);
		newCPU=accept_new_client("CPU", &cpuServer, &allSockets, cpus_conectadas);
		if(newCPU>0) log_info(kernel_log,"New CPU accepted with ID %d",newCPU);
		if (list_size(cpus_conectadas) > 0 && list_size(PCB_READY) > 0 ){
			round_robin();
		}
		// if (list_size(PCB_BLOCKED) > 0) sarasa(); // TODO coso
	}
	return 1;
}

int accept_new_client(char* what,int *server, fd_set *sockets,t_list *lista){
	int aceptado=0;
	char buffer_4[4];
	if (FD_ISSET(*server, &*sockets)){
		if ((aceptado=acceptConnection(*server)) < 1){
			log_error(kernel_log,"Error while trying to Accept() a new %s.",what);
		} else {
			maxSocket=aceptado;
			if (recv(aceptado, buffer_4, 1, 0) > 0){
				if (strncmp(buffer_4, "0",1) == 0){
					t_Client *cliente=malloc(sizeof(t_Client));
					cliente->clientID=aceptado;
					cliente->pid=0; /* only used for CPUs */
					cliente->status=0;
					list_add(lista, cliente);
					printf(" .:: New %s arriving (%d) ::.\n",what,list_size(lista));
				}
			} else {
				log_error(kernel_log,"Error while trying to read from a newly accepted %s.",what);
				killClient(aceptado,what);
				return -1;
			}
		}
	}
	return aceptado;
}

int accept_new_PCB(int newConsole){
	char PID[4];
	char buffer_4[4];
	sprintf(PID, "%04d", (int) newConsole);
	printf(" .:: NEW (0) program with PID=%s arriving ::.\n", PID);
	recv(newConsole, buffer_4, 4, 0);
	size_t ansisopLen=(size_t) atoi(buffer_4);
	char *code = malloc(ansisopLen);
	recv(newConsole, code, ansisopLen, 0);
	//uint32_t code_pages=requestPages2UMC(PID,ansisopLen,code,clientUMC);
	uint32_t code_pages=3;//TODO DELETE when using a real UMC
	if (code_pages>0){
		send(newConsole,PID,4,0);
		t_metadata_program* metadata = metadata_desde_literal(code);
		t_pcb *newPCB=malloc(sizeof(t_pcb));
		newPCB->pid=atoi(PID);
		newPCB->program_counter=metadata->instruccion_inicio;
		newPCB->stack_pointer=code_pages;
		newPCB->stack_index=queue_create();
		newPCB->status=READY;
		newPCB->instrucciones_size= metadata->instrucciones_size;
		newPCB->instrucciones_serializado = metadata->instrucciones_serializado;
		newPCB->etiquetas_size = metadata->etiquetas_size;
		newPCB->etiquetas = metadata->etiquetas;
		list_add(PCB_READY,newPCB);
		printf(" .:: The program with PID=%04d is now READY (%d) ::.\n", newPCB->pid, newPCB->status);
		printf("Consolas despues de aceptar: %d\n", list_size(PCB_READY)); // reemplazar queue por list
	} else {
		send(newConsole,"0000",4,0);
		printf(" .:: The program with PID=%s could not be started. System run out of memory ::.\n", PID);
		killClient(newConsole,"console");
	}
	free(code); // let it free
}

void round_robin(){
	t_Client *laCPU=list_remove(cpus_conectadas,0);
	char quantum[4];
	char quantum_sleep[4];
	void * pcb_buffer = NULL;
	void * tmp_buffer = NULL;
	size_t tmp_buffer_size = 0;
	size_t pcb_buffer_size = 0;
	if (list_size(PCB_READY) > 0){
		t_pcb *tuPCB;
		tuPCB=list_remove(PCB_READY,0);
		tuPCB->status=EXECUTING;
		laCPU->status=EXECUTING;
		laCPU->pid=tuPCB->pid;
		printf(" .:: Le mando a CPU %d el PCB del proceso %d ::.\n",laCPU->clientID, tuPCB->pid);
		sprintf(quantum, "%04d", setup.QUANTUM);
		sprintf(quantum_sleep, "%04d", setup.QUANTUM_SLEEP);
		serialize_pcb(tuPCB, &pcb_buffer, &pcb_buffer_size);
		tmp_buffer_size=1+4+4+4;
        tmp_buffer = malloc(tmp_buffer_size+pcb_buffer_size);
		asprintf(&tmp_buffer, "%d%s%s%04d", 1,quantum,quantum_sleep, pcb_buffer_size);
        memcpy(tmp_buffer+tmp_buffer_size, pcb_buffer, pcb_buffer_size);
        tmp_buffer_size = tmp_buffer_size + pcb_buffer_size;
		log_info(kernel_log,"Submitting to CPU %d the PID %d with pcb_buffer_size=%zu",laCPU->clientID, tuPCB->pid,pcb_buffer_size);
		log_info(kernel_log,"The PCB is %s",tmp_buffer);
		send(laCPU->clientID, tmp_buffer, tmp_buffer_size,0);
	}
	return;
}