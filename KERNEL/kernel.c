/* Kernel.c by pacevedo */
/* Compilame asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o kernel socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c kernel.c -L/usr/lib -lcommons -lparser-ansisop
 * El CPU compilalo asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu implementation_ansisop.c libs/socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c cpu.c -L/usr/lib -lcommons -lparser-ansisop -lm
 * La UMC compilala asi:
gcc -I/usr/include/commons -I/usr/include/commons/collections -o umc umc.c -L/usr/lib -pthread -lcommons
 * Y la consola asi:
gcc -o test_pablo_console socketCommons/socketCommons.c test_pablo_console.c
*/
#include "kernel.h"

/* BEGIN OF GLOBAL STUFF I NEED EVERYWHERE */
int consoleServer, cpuServer, clientUMC;
int maxSocket=0;
t_list	*PCB_READY, *PCB_BLOCKED, *PCB_EXIT; /* I think I don't need queues 4 NEW & EXECUTING */
t_list *cpus_conectadas, *consolas_conectadas, *solicitudes_io;
fd_set 	allSockets;
t_log* kernel_log;
/* END OF GLOBAL STUFF I NEED EVERYWHERE */

int main (int argc, char **argv){
	kernel_log = log_create("kernel.log", "Elestac-KERNEL", true, LOG_LEVEL_TRACE);
	if (start_kernel(argc, argv[1])<0) return 0; //load settings
	//clientUMC=connect2UMC();
	clientUMC=100;setup.PAGE_SIZE=1024; //TODO Delete -> lo hace connect2UMC()
	if (clientUMC<0){
		log_error(kernel_log, "Could not connect to the UMC. Please, try again.");
		return 0;
	}
	PCB_READY = list_create();
	PCB_BLOCKED = list_create();
	PCB_EXIT = list_create();
	cpus_conectadas = list_create();
	consolas_conectadas = list_create();
	solicitudes_io = list_create();
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
		/* BEGIN pueden cambiar en tiempo de ejecucion */
		setup.QUANTUM=config_get_int_value(config,"QUANTUM");
		setup.QUANTUM_SLEEP=config_get_int_value(config,"QUANTUM_SLEEP");
		/* END pueden cambiar en tiempo de ejecucion */
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

int requestPages2UMC(char* PID, int ansisopLen,char* code,int clientUMC){ //TODO Launch this in a thread
	char* buffer;
	char buffer_4[4];
	int bufferLen=1+4+4+4+ansisopLen; //1+PID+req_pages+size+code
	sprintf(buffer_4, "%04d", (int) (ansisopLen/setup.PAGE_SIZE)+1);
	asprintf(&buffer, "%d%s%s%04d%s", 1,PID,buffer_4, ansisopLen,code);
	send(clientUMC, buffer, bufferLen, 0);
	recv(clientUMC, buffer_4, 4, 0);
	int code_pages = atoi(buffer_4);
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
		break;
	default:
		printf("Otra senial\n");
		break;
	}
}

void add2FD_SET(void *client){
	t_Client *cliente=client;
	FD_SET(cliente->clientID, &allSockets);
}

void check_CPU_FD_ISSET(void *cpu){
	char *cpu_protocol = malloc(2);
	t_Client *laCPU = cpu;
	int pcb_size;
	void *pcb_serializado = NULL;
	char *tmp_buff = malloc(4);
	t_pcb *incomingPCB;
	if (FD_ISSET(laCPU->clientID, &allSockets)) {
		if (recv(laCPU->clientID, cpu_protocol, 1, 0) > 0){
			log_info(kernel_log,"CPU dijo: %s - Ejecutar protocolo correspondiente",cpu_protocol);
			switch (atoi(cpu_protocol)) {
			case 1: // Quantum finish
			case 2: // Program END
            case 3:				//3== IO
                recv(laCPU->clientID, tmp_buff, 4, 0);
				pcb_size=atoi(tmp_buff);
				recv(laCPU->clientID, pcb_serializado, pcb_size, 0);
     	        deserialize_pcb(&incomingPCB,&pcb_serializado,&pcb_size);
				switch (incomingPCB->status) {
				case READY:
					list_add(PCB_READY,incomingPCB);
					break;
				case BLOCKED:
					list_add(PCB_BLOCKED,incomingPCB);
					t_io *io_op = malloc(sizeof(t_io));
					io_op->pid=laCPU->pid;
					recv(laCPU->clientID, tmp_buff, 4, 0); // size of the io_name
					recv(laCPU->clientID, io_op->io_name, (size_t) atoi(tmp_buff), 0);
					recv(laCPU->clientID, io_op->io_units, 4, 0);
					list_add(solicitudes_io, io_op); // TODO check how we manage the different queues.
						// TODO I could create the i/o queues when loading the settings with a loop around the array.
					break;
				case EXIT:
					list_add(PCB_EXIT,incomingPCB);
					break;
				default:
					log_error(kernel_log,"Error with CPU protocol. Function check_CPU_FD_ISSET.");
				}
				laCPU->status=READY;
				laCPU->pid=0;
				list_add(cpus_conectadas, laCPU); /* return the CPU to the queue */
				break;
			case 4:				//4== semaforo
				//wait   [identificador de semáforo]
				//signal [identificador de semáforo]
				log_warning(kernel_log,"Error with CPU protocol and semaphore operation. Function check_CPU_FD_ISSET.");
				break;
			case 5:				//5== var compartida
				//obtener_valor [identificador de variable compartida]
				//grabar_valor  [identificador de variable compartida] [valor a grabar]
				log_warning(kernel_log,"Error with CPU protocol and shared variable operation. Function check_CPU_FD_ISSET.");
				break;
			default:
				log_error(kernel_log,"Caso no contemplado. CPU dijo: %s",cpu_protocol);
			}
			call_handlers();
		} else {
			printf(" .:: CPU %d has closed the connection ::. \n", laCPU->clientID);
			close(laCPU->clientID);
			bool getCPUIndex(void *nbr){
				t_Client *unCliente = nbr;
				bool matchea = (laCPU->clientID == unCliente->clientID);
				if (matchea){
					/* si matchea entonces matar el PCB y enviar error a consola */
				}
				return matchea;
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
	char *buffer_4=malloc(4);
	t_Client *cliente = console;
	if (FD_ISSET(cliente->clientID, &allSockets)) {
		if (recv(cliente->clientID, buffer_4, 2, 0) > 0){
			log_error(kernel_log,"Consola no deberia enviar nada pero dijo: %s",buffer_4);
		} else {
			char PID[4];
			sprintf(PID, "%04d", cliente->clientID);
			printf(" .:: A console has closed the connection, the associated PID %s will be terminated ::. \n", PID);
			/* Delete the PCB */
			bool match_PCB(void *pcb){
				t_pcb *unPCB = pcb;
				bool matchea = (cliente->clientID==unPCB->pid);
				if (matchea && unPCB->status==2){
					// if pcb is being held by CPU -> wait ! -> add it to the PCB_EXIT list
					/* find the CPU who's running this pcb and change cpu->status to EXIT
					 */
				}
				return matchea;
			}
			if(list_size(PCB_READY)>0)
				list_remove_and_destroy_by_condition(PCB_READY,match_PCB,destroy_PCB);
			if(list_size(PCB_BLOCKED)>0)
				list_remove_and_destroy_by_condition(PCB_BLOCKED,match_PCB,destroy_PCB);
			if(list_size(PCB_EXIT)>0)
				list_remove_and_destroy_by_condition(PCB_EXIT,match_PCB,destroy_PCB);
			close(cliente->clientID);
			bool getConsoleIndex(void *nbr){
				t_Client *unCliente = nbr;
				bool matchea= (cliente->clientID == unCliente->clientID);
				return matchea;
			}
			list_remove_by_condition(consolas_conectadas, getConsoleIndex);
			free(buffer_4);
		}
	}
}

int control_clients(){
	int newConsole,newCPU;
	struct timeval timeout = {.tv_sec = 1};
	FD_ZERO(&allSockets);
	FD_SET(cpuServer, &allSockets);
	FD_SET(consoleServer, &allSockets);
	list_iterate(consolas_conectadas,add2FD_SET);
	list_iterate(cpus_conectadas,add2FD_SET);
	int retval=select(maxSocket+1, &allSockets, NULL, NULL, &timeout); // (retval < 0) <=> signal
	if (retval>0) {
		list_iterate(consolas_conectadas,check_CONSOLE_FD_ISSET);
		list_iterate(cpus_conectadas,check_CPU_FD_ISSET);
		if ((newConsole=accept_new_client("console", &consoleServer, &allSockets, consolas_conectadas)) > 1)
			accept_new_PCB(newConsole);
		newCPU=accept_new_client("CPU", &cpuServer, &allSockets, cpus_conectadas);
		if(newCPU>0) log_info(kernel_log,"New CPU accepted with ID %d",newCPU);
	}
	call_handlers();
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
					cliente->pid=0;
					cliente->status=0;
					list_add(lista, cliente);
					printf(" .:: New %s arriving (%d) ::.\n",what,list_size(lista));
				}
			} else {
				log_error(kernel_log,"Error while trying to read from a newly accepted %s.",what);
				close(aceptado);
				return -1;
			}
		}
	}
	return aceptado;
}

void accept_new_PCB(int newConsole){
	char PID[4];
	char buffer_4[4];
	sprintf(PID, "%04d", newConsole);
	printf(" .:: NEW (0) program with PID=%s arriving ::.\n", PID);
	recv(newConsole, buffer_4, 4, 0);
	int ansisopLen = atoi(buffer_4);
	char *code = malloc(ansisopLen);
	recv(newConsole, code, ansisopLen, 0);
	//int code_pages=requestPages2UMC(PID,ansisopLen,code,clientUMC);
	int code_pages=3;//TODO DELETE when using a real UMC
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
		close(newConsole);
	}
	free(code); // let it free
}

void round_robin(){
	t_Client *laCPU=list_remove(cpus_conectadas,0);
	void * pcb_buffer = NULL;
	void * tmp_buffer = NULL;
	int tmp_buffer_size = 1+4+4+4; /* 1+QUANTUM+QUANTUM_SLEEP+PCB_SIZE */
	int pcb_buffer_size = 0;
	t_pcb *tuPCB;
	tuPCB=list_remove(PCB_READY,0);
	tuPCB->status=EXECUTING;
	laCPU->status=EXECUTING;
	laCPU->pid=tuPCB->pid;
	serialize_pcb(tuPCB, &pcb_buffer, &pcb_buffer_size);
	tmp_buffer = malloc((size_t) tmp_buffer_size+pcb_buffer_size);
	asprintf(&tmp_buffer, "%d%04d%04d%04d", 1,setup.QUANTUM,setup.QUANTUM_SLEEP, pcb_buffer_size);
	serialize_data(pcb_buffer, (size_t ) pcb_buffer_size, &tmp_buffer, &tmp_buffer_size );
	log_info(kernel_log,"Submitting to CPU %d the PID %d",laCPU->clientID, tuPCB->pid);
	send(laCPU->clientID, tmp_buffer, tmp_buffer_size,0);
}

void end_program() {

}

void process_io() {
	int i=0;
	while(strlen(setup.IO_ID[i])>0){
		// aca detectar el IO y procesar la solicitud
		i++;
	}
	// si llego aca hay exception -> el io_id esta mal
}

void call_handlers() {
	if(list_size(PCB_BLOCKED)>0) process_io();
	if(list_size(PCB_EXIT)>0) end_program();
	if (list_size(cpus_conectadas) > 0 && list_size(PCB_READY) > 0 ){
		round_robin();
	}
}