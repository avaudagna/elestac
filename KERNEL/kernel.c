/* Kernel.c by pacevedo */
#include <libs/pcb.h>
#include "kernel.h"

/* BEGIN OF GLOBAL STUFF I NEED EVERYWHERE */
int consoleServer, cpuServer, clientUMC, configFileFD;
int configFileWatcher;
int maxSocket=0;
char* configFileName;
char* configFilePath;
t_log   *kernel_log;
t_list  *PCB_READY, *PCB_BLOCKED, *PCB_EXIT;
t_list  *consolas_conectadas, *cpus_conectadas, *cpus_executing;
t_list **solicitudes_io;
fd_set 	 allSockets;
void* losDatosGlobales = NULL;
int losDatosGlobales_index;
/* END OF GLOBAL STUFF I NEED EVERYWHERE */

int main (int argc, char* *argv){
	kernel_log = log_create("kernel.log", "Elestac-KERNEL", true, LOG_LEVEL_TRACE);
	PCB_READY = list_create();
	PCB_BLOCKED = list_create();
	PCB_EXIT = list_create();
	cpus_conectadas = list_create();
	cpus_executing = list_create();
	consolas_conectadas = list_create();
	if (start_kernel(argc, argv[1])<0) return 0; //load settings
	clientUMC=connect2UMC();
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
	inotify_rm_watch(configFileFD, configFileWatcher);
	close(configFileFD);
	close(consoleServer);
	close(cpuServer);
	close(clientUMC);
	log_destroy(kernel_log);
	return 0;
}

int start_kernel(int argc, char* configFile){
	printf("\n\t\e[31;1m===========================================\e[0m\n");
	printf("\t.:: Vamo a calmarno que viene el Kernel ::.");
	printf("\n\t\e[31;1m===========================================\e[0m\n\n");
	if(argc==2){
		if (loadConfig(configFile)<0) {
			log_error(kernel_log, "Config file can not be loaded. Please, try again.");
			return -1;
		}else{
			configFileName = calloc(1, strlen(configFile));
			strcpy(configFileName, configFile);
			char cwd[1024];
			getcwd(cwd, sizeof(cwd));
			configFilePath = calloc(1, sizeof(cwd));
			strcpy(configFilePath, cwd);
		}
		configFileFD = inotify_init();
		configFileWatcher = inotify_add_watch(configFileFD, configFilePath, IN_MODIFY | IN_CREATE);
	}else{
		printf("\r Usage: ./kernel setup.conf \n");
		log_error(kernel_log, "Config file was not provided.");
		return -1;
	}
	signal (SIGINT, tratarSeniales);
	signal (SIGPIPE, tratarSeniales);
	return 0;
}

void* sem_wait_thread(void* cpuData){
	int semIndex, miID;
	int cpuData_index = 0;
	deserialize_data(&semIndex, sizeof(int), cpuData, &cpuData_index);
	deserialize_data(&miID, sizeof(int), cpuData, &cpuData_index);
	free(cpuData);
	log_info(kernel_log, "sem_wait_thread: WAIT semaphore %s by CPU %d started.", setup.SEM_ID[semIndex], miID);
	sem_wait(&semaforo_ansisop[semIndex]);
	send(miID, "0", 1, 0);
	log_info(kernel_log, "sem_wait_thread: WAIT semaphore %s by CPU %d finished.", setup.SEM_ID[semIndex], miID);
    pthread_exit(0);
}

int wait_coordination(int cpuID){
	bool semWait=false;
	int buffindex = 0;
	char* tmp_buff = calloc(1, sizeof(int));

	recv(cpuID, tmp_buff, 1, 0);
	if (strncmp(tmp_buff, "0", 1) == 0) semWait=true;

	recv(cpuID, tmp_buff, sizeof(int), 0);
	size_t SEM_ID_Size;
	deserialize_data(&SEM_ID_Size, sizeof(int), tmp_buff, &buffindex);

	char* SEM_ID = calloc(1, SEM_ID_Size);
	recv(cpuID, SEM_ID, SEM_ID_Size, 0);
	int semIndex = getSEMindex(SEM_ID);

	if(semWait){
		losDatosGlobales = calloc(1, sizeof(int)*2);
		losDatosGlobales_index = 0;
		serialize_data(&cpuID, (size_t) sizeof(int), &losDatosGlobales, &losDatosGlobales_index);
		serialize_data(&semIndex, (size_t) sizeof(int), &losDatosGlobales, &losDatosGlobales_index);
		pthread_t sem_thread;
		pthread_create(&sem_thread, NULL, sem_wait_thread, losDatosGlobales);
	}else{
		log_info(kernel_log, "wait_coordination: SIGNAL semaphore %s by CPU %d.", setup.SEM_ID[semIndex], cpuID);
		sem_post(&semaforo_ansisop[semIndex]);
	}
	free(tmp_buff);
	free(SEM_ID);
	return 1;
}
void* do_work(void *p){
	int tmp = *((int *) p);
	int miID = tmp;
	bool esTrue = true;
	log_info(kernel_log, "do_work: Starting the device %s with a sleep of %s milliseconds.", setup.IO_ID[miID], setup.IO_SLEEP[miID]);
	while(esTrue){
		sem_wait(&semaforo_io[miID]);
		t_io *io_op;
		if(list_size(solicitudes_io[miID]) > 0) {
			pthread_mutex_lock(&mut_io_list);
			io_op = list_remove(solicitudes_io[miID], 0);
			pthread_mutex_unlock(&mut_io_list);
			if (io_op->pid > 0) {
				log_info(kernel_log, "%s will perform %d operations.", setup.IO_ID[miID], atoi(io_op->io_units));
				int processing_io = atoi(setup.IO_SLEEP[miID]) * atoi(io_op->io_units) * 1000;
				usleep((useconds_t) processing_io);
				bool match_PCB(void *pcb) {
					t_pcb *unPCB = pcb;
					bool matchea = (io_op->pid == unPCB->pid);
					return matchea;
				}
				t_pcb *elPCB;
				elPCB = list_remove_by_condition(PCB_BLOCKED, match_PCB);
				if (elPCB->pid > 0) {
					elPCB->status = READY;
					list_add(PCB_READY, elPCB);
				} // Else -> The PCB was killed by end_program while performing I/O
				log_info(kernel_log, "do_work: Finished an io operation on device %s requested by PID %d.",
						 setup.IO_ID[miID], io_op->pid);
				free(io_op);
			}
		}
	}
	free(p);
}

int loadConfig(char* configFile){
	int counter, i = 0;
	unsigned int initValue;
	pthread_t *io_device;
	if (configFile == NULL)	return -1;
	t_config *config = config_create(configFile);
	log_info(kernel_log, "Loading settings.");
	if (config != NULL){
		setup.PUERTO_PROG = config_get_int_value(config,"PUERTO_PROG");
		setup.PUERTO_CPU = config_get_int_value(config,"PUERTO_CPU");
		setup.QUANTUM = config_get_int_value(config,"QUANTUM");
		setup.QUANTUM_SLEEP = config_get_int_value(config,"QUANTUM_SLEEP");
		setup.IO_ID = config_get_array_value(config,"IO_ID");
		setup.IO_SLEEP = config_get_array_value(config,"IO_SLEEP");
		counter=0; while(setup.IO_ID[counter]) counter++;
		setup.IO_COUNT = counter;
		solicitudes_io = realloc(solicitudes_io, counter * sizeof(t_list));
		semaforo_io = realloc(semaforo_io, counter * sizeof(sem_t));
		io_device = calloc(1, sizeof(pthread_t) * counter);
		int *thread_id = calloc(1, sizeof(int) * counter);
		for (i = 0; i < counter; i++) {
			solicitudes_io[i] = list_create();
			sem_init(&semaforo_io[i], 0, 0);
			thread_id[i] = i;
			pthread_create(&io_device[i],NULL,do_work, &thread_id[i]);
		}
		setup.SEM_ID=config_get_array_value(config,"SEM_ID");
		setup.SEM_INIT=config_get_array_value(config,"SEM_INIT");
		counter=0; while (setup.SEM_ID[counter]) counter++;
		semaforo_ansisop = realloc(semaforo_ansisop, counter * sizeof(sem_t));
		for (i = 0; i < counter; i++) {
			initValue = (uint) atoi(setup.SEM_INIT[i]);
			sem_init(&semaforo_ansisop[i], 0, initValue);
		}
		setup.SHARED_VARS=config_get_array_value(config,"SHARED_VARS");
		counter=0; while(setup.SHARED_VARS[counter]) counter++;
		setup.SHARED_VALUES = realloc(setup.SHARED_VALUES, counter * sizeof(int));
		for (i = 0; i < counter; i++) {
			setup.SHARED_VALUES[i]=0;
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
	char umcProtocol = '0';
	void* umcHandshake = calloc(1, sizeof(int) + sizeof(char));
	int umcHandshake_index = 0;
	void* umcHandshakeResponse = calloc(1, sizeof(int));
	int umcHandshakeResponse_index = 0;
	log_info(kernel_log, "Connecting to UMC on %s:%d.", setup.IP_UMC, setup.PUERTO_UMC);
	if (getClientSocket(&clientUMC, setup.IP_UMC, setup.PUERTO_UMC)) return (-1);
	serialize_data(&umcProtocol, (size_t) sizeof(char), &umcHandshake, &umcHandshake_index);
	serialize_data(&setup.STACK_SIZE, (size_t) sizeof(int), &umcHandshake, &umcHandshake_index);
	send(clientUMC, umcHandshake, (size_t) umcHandshake_index, 0);
	log_info(kernel_log, "Stack size (sent to UMC): %d.", setup.STACK_SIZE);
	if (recv(clientUMC, umcHandshakeResponse, sizeof(int), 0) < 0) return (-1);
	deserialize_data(&setup.PAGE_SIZE, sizeof(int), umcHandshakeResponse, &umcHandshakeResponse_index);
	log_info(kernel_log, "Page size: (received from UMC): %d.",setup.PAGE_SIZE);
	free(umcHandshake);
	free(umcHandshakeResponse);
	return clientUMC;
}

void *requestPages2UMC(void* request_buffer){
	int PID;
	int ansisopLen=0;
	char* code = NULL;
	int clientUMC = 0;
	int deserialize_index = 0;
	deserialize_data(&PID, sizeof(int), request_buffer, &deserialize_index);
	deserialize_data(&ansisopLen, sizeof(int), request_buffer, &deserialize_index);
	code = calloc(1,(size_t) ansisopLen);
	deserialize_data(code, (size_t) ansisopLen, request_buffer, &deserialize_index);
	deserialize_data(&clientUMC, sizeof(int), request_buffer, &deserialize_index);

	void* req2UMC = NULL;//1+PID+nbrOfPages+ansisopLen+code
	void* req2UMC_response = calloc(1, sizeof(int));
	char umcProtocol = '1';
	int nbrOfPages = ansisopLen/setup.PAGE_SIZE + 1;// +1 since ceil() does not work
	int req2UMC_index = 0;
	serialize_data(&umcProtocol, sizeof(char), &req2UMC, &req2UMC_index);
	serialize_data(&PID, sizeof(int), &req2UMC, &req2UMC_index);
	serialize_data(&nbrOfPages, sizeof(int), &req2UMC, &req2UMC_index);
	serialize_data(&ansisopLen, sizeof(int), &req2UMC, &req2UMC_index);
	serialize_data(code, (size_t) ansisopLen, &req2UMC, &req2UMC_index);
	log_info(kernel_log, "Requesting %d pages to UMC.", nbrOfPages);

	send(clientUMC, req2UMC, (size_t) req2UMC_index, 0);
	recv(clientUMC, req2UMC_response, sizeof(int), 0);
	int code_pages;
	int code_pages_index = 0;
	deserialize_data(&code_pages, sizeof(int), req2UMC_response, &code_pages_index);
	log_info(kernel_log, "UMC replied.");
	free(req2UMC);
    free(req2UMC_response);
	createNewPCB(PID, code_pages, code);
}
void tratarSeniales(int senial){
	printf("\n\t=============================================\n");
	printf("\t\tSystem received the signal: %d",senial);
	printf("\n\t=============================================\n");
	switch (senial){
		case SIGINT:
			// Detecta Ctrl+C y evita el cierre.
			printf("Me di cuenta que me tiraste la senial :8 \nDale Ctrl+C una vez más para confirmar.\n\n");
			signal (SIGINT, SIG_DFL); // solo controlo una vez.
			break;
		case SIGPIPE:
			// Trato de escribir en un socket que cerro la conexion.
			printf("La consola o el CPU con el que estabas hablando se murió. Llamá al 911.\n\n");
			break;
		default:
			printf("Me tiraste la senial\n");
			break;
	}
}

void add2FD_SET(void *client){
	t_Client *cliente=client;
	FD_SET(cliente->clientID, &allSockets);
}

t_pcb * recvPCB(int cpuID){
	char* tmp_buff = calloc(1, sizeof(int));
	int pcb_size;
	recv(cpuID, tmp_buff, (size_t) sizeof(int), 0);
	int pcb_size_index = 0;
	deserialize_data(&pcb_size, sizeof(int), tmp_buff, &pcb_size_index);
	void *pcb_serializado = calloc(1, (size_t) pcb_size);
	recv(cpuID, pcb_serializado, (size_t) pcb_size, 0);
	int pcb_serializado_index = 0;
	t_pcb* incomingPCB = calloc(1, sizeof(t_pcb));
	deserialize_pcb(&incomingPCB, pcb_serializado, &pcb_serializado_index);
	//testSerializedPCB(incomingPCB, pcb_serializado);
	free(tmp_buff);
	free(pcb_serializado);
	return incomingPCB;
}

void restoreCPU(t_Client *laCPU){
	bool getCPUIndex(void *nbr){
		t_Client *unaCPU = nbr;
		bool matchea = (laCPU->clientID == unaCPU->clientID);
		return matchea;
	}
	list_remove_by_condition(cpus_executing, getCPUIndex);
	laCPU->status = READY;
	laCPU->pid = 0;
	list_add(cpus_conectadas, laCPU); /* return the CPU to the queue */
}

void check_CPU_FD_ISSET(void *cpu){
	char* cpu_protocol = calloc(1, 1);
	char* tmp_buff = calloc(1, sizeof(int));
	int setValue = 0;
	int nameSize;
	int nameSize_index = 0;
	t_Client* laCPU = (t_Client*) cpu;
	if (laCPU != NULL && FD_ISSET(laCPU->clientID, &allSockets)) {
		log_debug(kernel_log,"CPU %d has something to say.", laCPU->clientID);
		if (recv(laCPU->clientID, cpu_protocol, 1, MSG_DONTWAIT) > 0){
			switch (atoi(cpu_protocol)){
				case 1:// Quantum end
				case 2:// Program END
					log_debug(kernel_log, "Receving a PCB");
					t_pcb* incomingPCB = recvPCB(laCPU->clientID);
					if (laCPU->status == EXIT || incomingPCB->status==EXIT || incomingPCB->status == BROKEN){
						list_add(PCB_EXIT, incomingPCB);
					}else{
						list_add(PCB_READY, incomingPCB);
					}
					restoreCPU(laCPU);
					break;
				case 3:// IO
					log_debug(kernel_log, "Receving an Input/Output request + a PCB");
					t_io *io_op = calloc(1,sizeof(t_io));
					io_op->pid = laCPU->pid;
					recv(laCPU->clientID, tmp_buff, sizeof(int), 0); // size of the io_name
					deserialize_data(&nameSize, sizeof(int), tmp_buff, &nameSize_index);
					io_op->io_name = calloc(1, (size_t) nameSize);
					io_op->io_units = calloc(1, sizeof(int));
					recv(laCPU->clientID, io_op->io_name, (size_t) nameSize, 0);
					recv(laCPU->clientID, io_op->io_units, sizeof(int), 0);
					io_op->io_index = getIOindex(io_op->io_name);
					t_pcb *blockedPCB = recvPCB(laCPU->clientID);
					if (io_op->io_index < 0 || laCPU->status == EXIT) {
						log_error(kernel_log, "AnSisOp program request an unplugged device or the console has been closed. #VamoACalmarno");
						list_add(PCB_EXIT,blockedPCB);
					} else {
						pthread_mutex_lock(&mut_io_list);
						list_add(solicitudes_io[io_op->io_index], io_op);
						pthread_mutex_unlock(&mut_io_list);
						list_add(PCB_BLOCKED, blockedPCB);
					}
					restoreCPU(laCPU);
					break;
				case 4:// semaforo
					log_debug(kernel_log, "Receving a semaphore operation from CPU %d", laCPU->clientID);
					if (wait_coordination(laCPU->clientID) == 1)
						log_debug(kernel_log, "Semaphore operation successfully handled.");
					break;
				case 5:// var compartida
					log_debug(kernel_log, "Receving a shared var operation from CPU %d", laCPU->clientID);
					recv(laCPU->clientID, tmp_buff, 1, 0);
					if (strncmp(tmp_buff, "1", 1) == 0) setValue = 1;
					recv(laCPU->clientID, tmp_buff, sizeof(int), 0);
					deserialize_data(&nameSize, sizeof(int), tmp_buff, &nameSize_index);
					char *theShared = calloc(1, (size_t) nameSize);
					recv(laCPU->clientID, theShared, (size_t) nameSize, 0);
					int sharedIndex = getSharedIndex(theShared);
					nameSize_index = 0; // for either serialization or deserialization
					if (setValue == 1) {
						recv(laCPU->clientID, tmp_buff, sizeof(int), 0);//recv & set the value
						int theVal;
						deserialize_data(&theVal, sizeof(int), tmp_buff, &nameSize_index);
						setup.SHARED_VALUES[sharedIndex] = theVal;
					} else {
						void* sharedValue = calloc(1, sizeof(int));
						serialize_data(&setup.SHARED_VALUES[sharedIndex], (size_t) sizeof(int), &sharedValue, &nameSize_index);
						send(laCPU->clientID, sharedValue, sizeof(int), 0); // send the value to the CPU
						free(sharedValue);
					}
					free(theShared);
					break;
				case 6:// imprimirValor
					log_debug(kernel_log, "Receving a value to print on console %d from CPU %d.", laCPU->pid, laCPU->clientID);
					int value2console;
					recv(laCPU->clientID, tmp_buff, sizeof(int), 0);
					deserialize_data(&value2console, sizeof(int), tmp_buff, &nameSize_index);
                    void* text2Console = NULL;
					int text2Console_index = 0;
					char consoleProtocol = '1';
					serialize_data(&consoleProtocol, (size_t) sizeof(char), &text2Console, &text2Console_index);
					serialize_data(&value2console, (size_t) sizeof(int), &text2Console, &text2Console_index);
					log_debug(kernel_log, "Console %d will print the value %d.", laCPU->pid, value2console);
					send(laCPU->pid, text2Console, (size_t) text2Console_index, 0); // send the value to the console
                    free(text2Console);
					break;
				case 7:// imprimirTexto
					log_debug(kernel_log, "Receving a text to print on console %d from CPU %d.", laCPU->pid, laCPU->clientID);
					recv(laCPU->clientID, tmp_buff, sizeof(int), 0);
					size_t txtSize;
					deserialize_data(&txtSize, sizeof(int), tmp_buff, &nameSize_index);
					char *theTXT = calloc(1, txtSize);
					recv(laCPU->clientID, theTXT, txtSize, 0);
					log_debug(kernel_log, "Console %d will print this text: %s.", laCPU->pid, theTXT);
					void* txt2console = NULL;
					char consoleProtocol2 = '2';
					int txt2console_index = 0;
					serialize_data(&consoleProtocol2, (size_t) sizeof(char), &txt2console, &txt2console_index);
					serialize_data(&txtSize, (size_t) sizeof(int), &txt2console, &txt2console_index);
					serialize_data(&theTXT, txtSize, &txt2console, &txt2console_index);
					send(laCPU->pid, txt2console, (size_t) txt2console_index, 0); // send the text to the console
					free(theTXT);
					free(txt2console);
					break;
				default:
					log_error(kernel_log,"Caso no contemplado. CPU dijo: %s",cpu_protocol);
			}
		}else{
			log_info(kernel_log,"CPU %d has closed the connection.", laCPU->clientID);
			close(laCPU->clientID);
			bool getCPUIndex(void *nbr){
				t_Client *unCliente = nbr;
				bool matchea = (laCPU->clientID == unCliente->clientID);
				if (matchea && laCPU->pid > 0)
					end_program(laCPU->pid, true, false, laCPU->status);
				return matchea;
			}
			if (list_size(cpus_conectadas) > 0)
				list_remove_by_condition(cpus_conectadas, getCPUIndex);
			if (list_size(cpus_executing) > 0)
				list_remove_by_condition(cpus_executing, getCPUIndex);
		}
		RoundRobinReport();
	}
	free(cpu_protocol);
	free(tmp_buff);
}

void destroy_PCB(void *pcb){
	t_pcb *unPCB = pcb;
	free(unPCB);
}

void check_CONSOLE_FD_ISSET(void *console){
	void* ConBuff = calloc(1, 1);
	t_Client *cliente = console;
	if (FD_ISSET(cliente->clientID, &allSockets)) {
		if (recv(cliente->clientID, ConBuff, 1, 0) == 0){
			log_info(kernel_log,"A console has closed the connection, the associated PID %04d will be terminated.", cliente->clientID);
			end_program(cliente->clientID, false, true, BROKEN);
		}
	}
	free(ConBuff);
}

int control_clients(){
	int newConsole,newCPU;
	struct timeval timeout = {.tv_sec = 1};
	FD_ZERO(&allSockets);
	FD_SET(cpuServer, &allSockets);
	FD_SET(consoleServer, &allSockets);
	FD_SET(configFileFD, &allSockets);
	list_iterate(consolas_conectadas,add2FD_SET);
	list_iterate(cpus_conectadas,add2FD_SET);
	list_iterate(cpus_executing,add2FD_SET);
	int retval = select(maxSocket+1, &allSockets, NULL, NULL, &timeout); // (retval < 0) <=> signal
	if (retval>0) {
		if (FD_ISSET(configFileFD, &allSockets)) {
			char configFileBuff[EVENT_BUF_LEN];
			ssize_t limit = read(configFileFD, configFileBuff, EVENT_BUF_LEN);
			if (limit > 0 ){
				int base = 0;
				while (base < limit ) {
					struct inotify_event *event = ( struct inotify_event * ) &configFileBuff[base];
					if ((strcmp(event->name, configFileName)==0) & ((event->mask & IN_CREATE) || (event->mask & IN_MODIFY))){
						t_config *config = config_create(configFileName);
						if (config != NULL && config_has_property(config,"QUANTUM") && config_has_property(config,"QUANTUM_SLEEP")){
							setup.QUANTUM = config_get_int_value(config,"QUANTUM");
							setup.QUANTUM_SLEEP = config_get_int_value(config,"QUANTUM_SLEEP");
							config_destroy(config);
							log_info(kernel_log, "New config file loaded. QUANTUM=%d & QUANTUM_SLEEP=%d.", setup.QUANTUM, setup.QUANTUM_SLEEP);
						}
					}
					base += EVENT_SIZE + event->len;
				}
			}
		}
		if (list_size(consolas_conectadas) > 0)
			list_iterate(consolas_conectadas, check_CONSOLE_FD_ISSET);
		if (list_size(cpus_conectadas) > 0)
			list_iterate(cpus_conectadas, check_CPU_FD_ISSET);
		if (list_size(cpus_executing) > 0)
			list_iterate(cpus_executing, check_CPU_FD_ISSET);
		if ((newConsole=accept_new_client("console", &consoleServer, &allSockets, consolas_conectadas)) > 1){
			accept_new_PCB(newConsole);
			RoundRobinReport();
		}
		newCPU=accept_new_client("CPU", &cpuServer, &allSockets, cpus_conectadas);
		if(newCPU>0) log_info(kernel_log,"New CPU accepted with ID %d",newCPU);
	}
	call_handlers();
	return 1;
}

int accept_new_client(char* what,int *server, fd_set *sockets,t_list *lista){
	int aceptado=0;
	char newBuff[1];
	if (FD_ISSET(*server, &*sockets)){
		if ((aceptado=acceptConnection(*server)) < 1){
			log_error(kernel_log,"Error while trying to Accept() a new %s.",what);
		} else {
			maxSocket=aceptado;
			if (recv(aceptado, newBuff, 1, 0) > 0){
				if (strncmp(newBuff, "0", 1) == 0){
					t_Client *cliente = calloc(1, sizeof(t_Client));
					cliente->clientID = aceptado;
					cliente->pid = 0;
					cliente->status = 0;
					list_add(lista, cliente);
					log_info(kernel_log, "New %s arriving (Total %s connections: %d).", what, what, list_size(lista));
				}
			}else{
				log_error(kernel_log,"Error while trying to read from a newly accepted %s.",what);
				close(aceptado);
				return -1;
			}
		}
	}
	return aceptado;
}

void accept_new_PCB(int newConsole){
	void* ansisopLenBuff = calloc(1, sizeof(int));
	int ansisopLenBuff_index = 0;
	int ansisopLen;
	log_info(kernel_log, "NEW (0) program with PID=%04d arriving.", newConsole);
	recv(newConsole, ansisopLenBuff, sizeof(int), 0);
	deserialize_data(&ansisopLen, sizeof(int), ansisopLenBuff, &ansisopLenBuff_index);
	char *code = calloc(1,(size_t) ansisopLen);
	recv(newConsole, code, (size_t) ansisopLen, 0);
	void* request_buffer = NULL;
	int request_buffer_index = 0;
	serialize_data(&newConsole, sizeof(int), &request_buffer, &request_buffer_index);
	serialize_data(&ansisopLen, sizeof(int), &request_buffer, &request_buffer_index);
	serialize_data(code, (size_t) ansisopLen, &request_buffer, &request_buffer_index);
	serialize_data(&clientUMC, sizeof(int), &request_buffer, &request_buffer_index);
	pthread_t newPCB_thread;
	pthread_create(&newPCB_thread, NULL, requestPages2UMC, request_buffer);
	free(code);
	free(ansisopLenBuff);
}

void createNewPCB(int newConsole, int code_pages, char* code){
	void* PIDserializado = calloc(1, sizeof(int));
	int PIDserializado_index = 0;
	if (code_pages>0){
		serialize_data(&newConsole, sizeof(int), &PIDserializado, &PIDserializado_index);
		log_info(kernel_log, "Pages of code + stack = %d.", code_pages);
		send(newConsole, PIDserializado, sizeof(int), 0);
		t_metadata_program* metadata = metadata_desde_literal(code);
		t_pcb *newPCB = calloc(1, sizeof(t_pcb));
		newPCB->pid = newConsole;
		newPCB->program_counter = metadata->instruccion_inicio;
		newPCB->stack_pointer = code_pages * setup.PAGE_SIZE;
		newPCB->stack_index = queue_create();
		newPCB->status = READY;
		newPCB->instrucciones_size = metadata->instrucciones_size;
		newPCB->instrucciones_serializado = metadata->instrucciones_serializado;
		newPCB->etiquetas_size = metadata->etiquetas_size;
        newPCB->etiquetas = calloc(1, metadata->etiquetas_size);
		memcpy(newPCB->etiquetas, metadata->etiquetas, metadata->etiquetas_size);
        free(metadata);
		list_add(PCB_READY, newPCB);
		log_info(kernel_log, "The program with PID=%04d is now READY (%d).", newPCB->pid, newPCB->status);
	} else {
		int cero = 0;
		serialize_data(&cero, sizeof(int), &PIDserializado, &PIDserializado_index);
		send(newConsole, PIDserializado, sizeof(int), 0);
		log_error(kernel_log, "The program with PID=%04d could not be started. System run out of memory.", newConsole);
		close(newConsole);
	}
	free(PIDserializado);
}

void round_robin(){
	char uno = '1';
	int tmp_buffer_size = 0;
	int pcb_buffer_size = 0;
	void* pcb_buffer = NULL;
	void* tmp_buffer = NULL;
	t_Client *laCPU = list_remove(cpus_conectadas,0);
	t_pcb    *tuPCB = list_remove(PCB_READY,0);
	tuPCB->status = EXECUTING;
	laCPU->status = EXECUTING;
	laCPU->pid = tuPCB->pid;
	serialize_pcb(tuPCB, &pcb_buffer, &pcb_buffer_size);
	serialize_data(&uno, sizeof(char), &tmp_buffer, &tmp_buffer_size);
	serialize_data(&setup.QUANTUM, sizeof(int), &tmp_buffer, &tmp_buffer_size);
	serialize_data(&setup.QUANTUM_SLEEP, sizeof(int), &tmp_buffer, &tmp_buffer_size);
	serialize_data(&pcb_buffer_size, sizeof(int), &tmp_buffer, &tmp_buffer_size);
	serialize_data(pcb_buffer, (size_t ) pcb_buffer_size, &tmp_buffer, &tmp_buffer_size);
	log_info(kernel_log,"Submitting to CPU %d the PID=%04d.", laCPU->clientID, tuPCB->pid);
	send(laCPU->clientID, tmp_buffer, (size_t) tmp_buffer_size, 0);
	list_add(cpus_executing,laCPU);
	free(tmp_buffer);
	free(pcb_buffer);
    free(tuPCB->etiquetas);
	free(tuPCB);
}

void RoundRobinReport(){
	log_info(kernel_log, "Round Robin report:");
	int nBLOCKED = list_size(PCB_BLOCKED);
	int nEXIT = list_size(PCB_EXIT);
	int nEXEC = list_size(cpus_executing);
	int nREADY = list_size(PCB_READY);
	int nNEW = list_size(consolas_conectadas) - nREADY - nEXEC - nBLOCKED - nEXIT;
	log_info(kernel_log, "NEW=%d, READY=%d, EXECUTING=%d, BLOCKED=%d, EXIT=%d.", nNEW, nREADY, nEXEC, nBLOCKED, nEXIT);
}

void end_program(int pid, bool consoleStillOpen, bool cpuStillOpen, int status) { /* Search everywhere for the PID and kill it ! */
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
	char dos = '2';
	void* umcKillProg = NULL;
	int umcKillProg_index = 0;
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

	if (cpuStillOpen) {
		if (list_size(cpus_executing) > 0) {
			bool getCPUIndex(void *nbr) {
				t_Client *aCPU = nbr;
				return (pid == aCPU->pid);
			}
			t_Client *theCPU;
			theCPU = list_find(cpus_executing, getCPUIndex);
			theCPU->status = EXIT;
		}
	} else {
		pcb_found = true;
	}

	if (pcb_found == true) {
		serialize_data(&dos, sizeof(char), &umcKillProg, &umcKillProg_index);
		serialize_data(&pid, sizeof(int), &umcKillProg, &umcKillProg_index);
		send(clientUMC, umcKillProg, (size_t) umcKillProg_index, 0);
		log_info(kernel_log, "Program %04d has been terminated", pid);
		free(umcKillProg);
	}
	if (consoleStillOpen){
		int finalizar = 0;
		void* consoleKillProg = NULL;
		int consoleKillProg_index = 0;
		if(status == BROKEN) finalizar = 3;
		log_info(kernel_log, "Program status was %d. Console will inform this properly to the user.", status);
		serialize_data(&finalizar, sizeof(int), &consoleKillProg, &consoleKillProg_index);
		send(pid, consoleKillProg, sizeof(int), 0); // send exit code to console
		free(consoleKillProg);
	}
	close(pid); // close console socket
	bool getConsoleIndex(void *nbr) {
		t_Client *unCliente = nbr;
		return (pid == unCliente->clientID);
	}
	list_remove_by_condition(consolas_conectadas, getConsoleIndex);
}

void process_io() {
	int i;
	for (i = 0; i < setup.IO_COUNT; i++) {
		if (list_size(solicitudes_io[i]) > 0)
			sem_post(&semaforo_io[i]);
	}
}

int getIOindex(char *io_name) {
	int i;
	for (i = 0; i < setup.IO_COUNT; i++) {
		if (strcmp(setup.IO_ID[i],io_name) == 0)
			return i;
	}
	return -1;
}

int getSEMindex(char *sem_id) {
	int i=0;
	while(setup.SEM_ID){
		if (strcmp(setup.SEM_ID[i], sem_id) == 0)
			return i;
		i++;
	}
	return -1;
}

int getSharedIndex(char *shared_name) {
	int i = 0;
	while (setup.SHARED_VARS[i]){
		if (strcmp(setup.SHARED_VARS[i],shared_name) == 0)
			return i;
		i++;
	}
	return -1;
}

void call_handlers() {
	while (list_size(PCB_EXIT) > 0) {
		t_pcb *elPCB;
		elPCB = list_get(PCB_EXIT, 0);
		end_program(elPCB->pid, true, false, elPCB->status);
	}
	if (list_size(PCB_BLOCKED) > 0) process_io();
	while (list_size(cpus_conectadas) > 0 && list_size(PCB_READY) > 0 ) round_robin();
}

// C'est tout