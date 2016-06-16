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
t_log   *kernel_log;
t_list  *PCB_READY, *PCB_BLOCKED, *PCB_EXIT;
t_list  *consolas_conectadas, *cpus_conectadas, *cpus_executing;
t_list **solicitudes_io;
fd_set 	 allSockets;
/* END OF GLOBAL STUFF I NEED EVERYWHERE */

int main (int argc, char **argv){
	kernel_log = log_create("kernel.log", "Elestac-KERNEL", true, LOG_LEVEL_TRACE);
	PCB_READY = list_create();
	PCB_BLOCKED = list_create();
	PCB_EXIT = list_create();
	cpus_conectadas = list_create();
	cpus_executing = list_create();
	consolas_conectadas = list_create();
	if (start_kernel(argc, argv[1])<0) return 0; //load settings
	//clientUMC=connect2UMC();
	clientUMC=100;setup.PAGE_SIZE=1024; //TODO Delete -> lo hace connect2UMC()
	if (clientUMC<0){
		log_error(kernel_log, "Could not connect to the UMC. Please, try again.");
		return 0;
	}
	if(setServerSocket(&consoleServer, setup.KERNEL_IP, setup.PUERTO_PROG)<0){
		log_error(kernel_log,"Error while creating the CONSOLE server.");
		return 0;
	}
	if(setServerSocket(&cpuServer, setup.KERNEL_IP, setup.PUERTO_CPU)<0){
		log_error(kernel_log,"Error while creating the CPU server.");
		return 0;
	}
	maxSocket=cpuServer;
	log_info(kernel_log,"Servers to CPUs and consoles up and running. Waiting for incoming connections.");
	while (control_clients());
	log_error(kernel_log, "Closing kernel.");
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

void* do_work(void *p) { // TODO cuando se crea io_op es NULL
	int miID = *((int *) p);
	t_io *io_op;
	log_info(kernel_log, "do_work: Starting the device %s with a sleep of %s milliseconds\n", setup.IO_ID[miID], setup.IO_SLEEP[miID]);
	while(1){
		sem_wait(&semaforo_io[miID]);
		log_info(kernel_log, "A new I/O request arrived at %s", setup.IO_ID[miID]);
		pthread_mutex_lock(&mut_io_list);
		io_op = list_remove(solicitudes_io[miID], 0);
		pthread_mutex_unlock(&mut_io_list);
		if (io_op != NULL){
			log_info(kernel_log,"%s will perform %s operations.", setup.IO_ID[miID], io_op->io_units);
			int processing_io = atoi(setup.IO_SLEEP[miID]) * atoi(io_op->io_units) * 1000;
			// TODO Carefull here !! usleep() may fail when processing_io is > 1 sec.
			// TODO Example provided in setup.data has delays bigger than 1 sec.
			usleep((useconds_t) processing_io);
			int pid = io_op->pid;
			bool match_PCB(void *pcb){
				t_pcb *unPCB = pcb;
				bool matchea = (pid==unPCB->pid);
				return matchea;
			}
			t_pcb *elPCB;
			elPCB = list_remove_by_condition(PCB_BLOCKED, match_PCB);
			if (elPCB != NULL ){
				elPCB->status = READY;
				list_add(PCB_READY,elPCB);
			} // Else -> The PCB was killed by end_program while performing I/O
			log_info(kernel_log, "do_work: Finished an io operation on device %s requested by PID %d\n", setup.IO_ID[miID], io_op->pid);
		}
	}
}

int loadConfig(char* configFile){
	int counter = 0, i = 0;
	pthread_t *io_device;
	if (configFile == NULL)	return -1;
	t_config *config = config_create(configFile);
	log_info(kernel_log, "Loading settings\n");
	if (config != NULL){
		setup.PUERTO_PROG = config_get_int_value(config,"PUERTO_PROG");
		setup.PUERTO_CPU = config_get_int_value(config,"PUERTO_CPU");
		/* BEGIN pueden cambiar en tiempo de ejecucion */
		setup.QUANTUM = config_get_int_value(config,"QUANTUM");
		setup.QUANTUM_SLEEP = config_get_int_value(config,"QUANTUM_SLEEP");
		/* END pueden cambiar en tiempo de ejecucion */
		setup.IO_ID = config_get_array_value(config,"IO_ID");
		setup.IO_SLEEP = config_get_array_value(config,"IO_SLEEP");
		while(setup.IO_ID[counter])
			counter++;
		setup.IO_COUNT = counter;
		solicitudes_io = realloc(solicitudes_io, counter * sizeof(t_list));
		semaforo_io = realloc(semaforo_io, counter * sizeof(sem_t));
		io_device = malloc(sizeof(pthread_t) * counter);
		for (i = 0; i < counter; i++) {
			solicitudes_io[i] = list_create();
			sem_init(&semaforo_io[i], 0, 0);
			int *thread_id = malloc(sizeof(int));
			*thread_id=i;
			pthread_create(&io_device[i],NULL,do_work, thread_id);
		}
		setup.SEM_ID=config_get_array_value(config,"SEM_ID");
		setup.SEM_INIT=config_get_array_value(config,"SEM_INIT");
		counter=0;
		setup.SHARED_VARS=config_get_array_value(config,"SHARED_VARS");
		while(setup.SHARED_VARS[counter])
			counter++;
		setup.SHARED_VALUES = realloc(setup.SHARED_VALUES, counter * sizeof(int));
		for (i = 0; i < counter; i++) {
			setup.SHARED_VALUES[counter]=0;
		}
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
	sprintf(buffer_4, "%04d", (ansisopLen/setup.PAGE_SIZE)+1);
	asprintf(&buffer, "%d%s%s%04d%s", 1,PID,buffer_4, ansisopLen,code);
	send(clientUMC, buffer, (size_t) bufferLen, 0);
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
	int setValue = 0;
	t_Client *laCPU = cpu;
	int pcb_size;
	void *pcb_serializado = NULL;
	char *tmp_buff = malloc(4);
	t_pcb *incomingPCB;
	if (FD_ISSET(laCPU->clientID, &allSockets)) {
		if (recv(laCPU->clientID, cpu_protocol, 1, 0) > 0){
			log_info(kernel_log,"CPU dijo: %s - Ejecutar protocolo correspondiente",cpu_protocol);
			switch (atoi(cpu_protocol)) {
			case 1:// Quantum end
			case 2:// Program END
            case 3:// IO
                recv(laCPU->clientID, tmp_buff, 4, 0);
				pcb_size=atoi(tmp_buff);
				recv(laCPU->clientID, pcb_serializado, (size_t) pcb_size, 0);
     	        deserialize_pcb(&incomingPCB,&pcb_serializado,&pcb_size);
				t_io *io_op = malloc(sizeof(t_io));
				switch(incomingPCB->status) {
				case EXIT:
					list_add(PCB_EXIT,incomingPCB);
					break;
				case BLOCKED:
					io_op->pid=laCPU->pid;
					recv(laCPU->clientID, tmp_buff, 4, 0); // size of the io_name
					recv(laCPU->clientID, io_op->io_name, (size_t) atoi(tmp_buff), 0);
					recv(laCPU->clientID, io_op->io_units, 4, 0);
					io_op->io_index = getIOindex(io_op->io_name);
					if(io_op->io_index < 0 || laCPU->status == EXIT){
						log_error(kernel_log, "AnSisOp program request an unplugged device or the console has been closed. #VamoACalmarno");
						list_add(PCB_EXIT,incomingPCB);
					}else{
						pthread_mutex_lock(&mut_io_list);
						list_add(solicitudes_io[io_op->io_index], io_op);
						pthread_mutex_unlock(&mut_io_list);
						list_add(PCB_BLOCKED,incomingPCB);
					}
					break;
				case READY:
					if (laCPU->status == EXIT){
						list_add(PCB_EXIT, incomingPCB);
					} else {
						list_add(PCB_READY, incomingPCB);
					}
					break;
				default:
					log_error(kernel_log, "recv() a broken PCB");
				}
				bool getCPUIndex(void *nbr){
					t_Client *unaCPU = nbr;
					bool matchea = (laCPU->clientID == unaCPU->clientID);
					return matchea;
				}
				list_remove_by_condition(cpus_executing, getCPUIndex);
				laCPU->status = READY;
				laCPU->pid = 0;
				list_add(cpus_conectadas, laCPU); /* return the CPU to the queue */
				break;
			case 4:// semaforo
				//wait   [identificador de semáforo]
				//signal [identificador de semáforo]
				log_warning(kernel_log,"Error with CPU protocol and semaphore operation. Function check_CPU_FD_ISSET.");
				break;
			case 5:// var compartida
				recv(laCPU->clientID, tmp_buff, 1, 0);
				if (strncmp(tmp_buff, "1",1) == 0) setValue = 1;
				recv(laCPU->clientID, tmp_buff, 4, 0);
				size_t varNameSize = (size_t) atoi(tmp_buff);
				char *theShared = malloc(varNameSize);
				recv(laCPU->clientID, theShared, varNameSize, 0);
				int sharedIndex = getSharedIndex(theShared);
				if (setValue == 1) {
					recv(laCPU->clientID, tmp_buff, 4, 0);//recv & set the value
					int theVal = atoi(tmp_buff);
					setup.SHARED_VALUES[sharedIndex] = theVal;
				} else {
					char *sharedValue = malloc(4);
					sprintf(sharedValue, "%04d", setup.SHARED_VALUES[sharedIndex]);
					send(laCPU->clientID, sharedValue, 4, 0); // send the value to the CPU
					free(sharedValue);
				}
				free(theShared);
				break;
			case 6:// imprimirValor
				recv(laCPU->clientID, tmp_buff, 4, 0);
				size_t nameSize = (size_t) atoi(tmp_buff);
				char *theName = malloc(nameSize);
				recv(laCPU->clientID, theName, nameSize, 0);
				recv(laCPU->clientID, tmp_buff, 4, 0);
				char *value2console = malloc(1+4+nameSize+4);
				asprintf(&value2console, "%d%04d%s%04d", 1, nameSize, theName, tmp_buff);//1+nameSize+name+value
				send(laCPU->pid, value2console, (9+nameSize), 0); // send the value to the console
				free(theName);
				free(value2console);
				break;
			case 7:// imprimirTexto
				recv(laCPU->clientID, tmp_buff, 4, 0);
				size_t txtSize = (size_t) atoi(tmp_buff);
				char *theTXT = malloc(txtSize);
				recv(laCPU->clientID, theTXT, txtSize, 0);
				char *txt2console = malloc(1+4+txtSize);
				asprintf(&txt2console, "%d%04d%s", 2, txtSize, theTXT);
				send(laCPU->pid, txt2console, (5+txtSize), 0); // send the text to the console
				free(theTXT);
				free(txt2console);
				break;
			default:
				log_error(kernel_log,"Caso no contemplado. CPU dijo: %s",cpu_protocol);
			}
			call_handlers();
		} else {
			log_info(kernel_log," .:: CPU %d has closed the connection ::. \n", laCPU->clientID);
			close(laCPU->clientID);
			bool getCPUIndex(void *nbr){
				t_Client *unCliente = nbr;
				bool matchea = (laCPU->clientID == unCliente->clientID);
				if (matchea){
					end_program(laCPU->pid, true, false);
				}
				return matchea;
			}
			if (list_size(cpus_conectadas) > 0)
				list_remove_by_condition(cpus_conectadas, getCPUIndex);
			if (list_size(cpus_executing) > 0)
				list_remove_by_condition(cpus_executing, getCPUIndex);
		}
	}
	free(cpu_protocol);
	free(tmp_buff);
}

void destroy_PCB(void *pcb){
	t_pcb *unPCB = pcb;
	free(unPCB);
}

void check_CONSOLE_FD_ISSET(void *console){
	char *buffer_4=malloc(4);
	t_Client *cliente = console;
	if (FD_ISSET(cliente->clientID, &allSockets)) {
		if (recv(cliente->clientID, buffer_4, 2, 0) > 0){
			log_error(kernel_log,"Consola no deberia enviar nada pero dijo: %s",buffer_4);
		} else {
			log_info(kernel_log," .:: A console has closed the connection, the associated PID %04d will be terminated ::. \n", cliente->clientID);
			end_program(cliente->clientID, false, true);
		}
	}
	free(buffer_4);
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
					cliente->status = 0;
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
	int       tmp_buffer_size = 1+4+4+4; /* 1+QUANTUM+QUANTUM_SLEEP+PCB_SIZE */
	int       pcb_buffer_size = 0;
	void     *pcb_buffer = NULL;
	void     *tmp_buffer = NULL;
	t_Client *laCPU = list_remove(cpus_conectadas,0);
	t_pcb    *tuPCB = list_remove(PCB_READY,0);
	tuPCB->status = EXECUTING;
	laCPU->status = EXECUTING;
	laCPU->pid = tuPCB->pid;
	serialize_pcb(tuPCB, &pcb_buffer, &pcb_buffer_size);
	tmp_buffer = malloc((size_t) tmp_buffer_size+pcb_buffer_size);
	asprintf(&tmp_buffer, "%d%04d%04d%04d", 1, setup.QUANTUM, setup.QUANTUM_SLEEP, pcb_buffer_size);
	serialize_data(pcb_buffer, (size_t ) pcb_buffer_size, &tmp_buffer, &tmp_buffer_size );
	log_info(kernel_log,"Submitting to CPU %d the PID %d", laCPU->clientID, tuPCB->pid);
	send(laCPU->clientID, tmp_buffer, tmp_buffer_size, 0);
	free(tmp_buffer);
	list_add(cpus_executing,laCPU);
}

void end_program(int pid, bool consoleStillOpen, bool cpuStillOpen) { /* Search everywhere for the PID and kill it ! */
/*
 * CASOS
 *
 * 1) CPU cierra la conexion -> no tengo el PCB en ningun lado, tengo el PID
 *      a) Avisar a UMC
 *      b) Avisar a consola
 * 2) Consola cierra la conexion -> tengo el PID
 *      a) Avisar a UMC
 *      b) Buscar PCB y borrarlo
 *          i) puede estar ready, blocked o exit (casos lindos)
 *          ii) puede estar executing -> cambiar status de CPU
 *          iii) la CPU puede haber muerto -> matar socket de consola y listo
 * 3) Programa termina, hay error en ejecución de CPU o problema en UMC -> esta en PCB_EXIT
 *      a) Avisar a UMC
 *      b) Buscar PCB y borrarlo
 *          i) seguro está en exit (pero no pierdo nada con buscar en los otros)
 *      c) Avisar a consola
 * 4) It's new... (probably waiting for UMC to reply with requested pages)
 *      a) Puedo ignorar este caso si no cierro el socket con consola... en la próxima vuelta el PCB va a estar en PCB_READY.
 */
	char* buffer=malloc(5);
	bool pcb_found = false;
	bool match_PCB(void *pcb){
		t_pcb *unPCB = pcb;
		bool matchea = (pid==unPCB->pid);
		if (matchea)
			pcb_found = true;
		return matchea;
	}
	if (list_size(PCB_READY) > 0)
		list_remove_and_destroy_by_condition(PCB_READY,match_PCB,destroy_PCB);
	if (list_size(PCB_BLOCKED) > 0)
		list_remove_and_destroy_by_condition(PCB_BLOCKED,match_PCB,destroy_PCB);
	if (list_size(PCB_EXIT) > 0)
		list_remove_and_destroy_by_condition(PCB_EXIT,match_PCB,destroy_PCB);

	if (cpuStillOpen){
		if (list_size(cpus_executing) > 0){
			bool getCPUIndex(void *nbr) {
				t_Client *aCPU = nbr;
				return (pid == aCPU->pid);
			}
			t_Client *theCPU;
			theCPU = list_find(cpus_executing, getCPUIndex);
			theCPU->status = EXIT;
		}
	}else{
		pcb_found = true;
	}

	if (pcb_found == true) {
		sprintf(buffer, "%d%04d", 2, pid);
		send(clientUMC, buffer, 5, 0);
		if (consoleStillOpen) send(pid, "0", 1, 0); // send exit code to console
		close(pid); // close console socket
		bool getConsoleIndex(void *nbr) {
			t_Client *unCliente = nbr;
			return (pid == unCliente->clientID);
		}
		list_remove_by_condition(consolas_conectadas, getConsoleIndex);
	}
	free(buffer);
}

void process_io() {
	int i;
	for (i = 0; i < setup.IO_COUNT; i++) {
		if (list_size(solicitudes_io[i]) > 0) {
			sem_post(&semaforo_io[i]);
		}
	}
}

int getIOindex(char *io_name) {
	int i;
	for (i = 0; i < setup.IO_COUNT; i++) {
		if (strcmp(setup.IO_ID[i],io_name) == 0) {
			return i;
		}
	}
	return -1;
}

int getSharedIndex(char *shared_name) {
	int i = 0;
	while (setup.SHARED_VARS[i]){
		if (strcmp(setup.SHARED_VARS[i],shared_name) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

void call_handlers() {
	while (list_size(PCB_EXIT) > 0) {
		t_pcb *elPCB;
		elPCB = list_get(PCB_EXIT, 0);
		end_program(elPCB->pid, true, true);
	}
	if (list_size(PCB_BLOCKED) > 0) process_io();
	while (list_size(cpus_conectadas) > 0 && list_size(PCB_READY) > 0 ) round_robin();
}