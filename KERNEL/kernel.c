/* Kernel.c by pacevedo */
/* Compilame asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o kernel socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c kernel.c -L/usr/lib -lcommons -lparser-ansisop
*/
#include "kernel.h"

/* BEGIN OF GLOBAL STUFF I NEED EVERYWHERE */
int consoleServer, cpuServer;
int consoleSockets[MAX_CLIENTS]={0};
int cpuSockets[MAX_CLIENTS]={0};
int cpusOnline=0;
int consolesOnline=0;
int maxSocket=0;
int maxCPUs=0;
int maxConsoles=0;
t_list* PCB_NEW_list;
t_list* cpus_conectadas;
fd_set 	allSockets,
		checkClientCtrlC;
/* END OF GLOBAL STUFF I NEED EVERYWHERE */

int main (int argc, char **argv) {
	if(start_kernel(argc, argv[1])<0) return 0; //load settings
	//int clientUMC=connect2UMC();
	int clientUMC=100;setup.PAGE_SIZE=1024; //TODO Delete -> lo hace connect2UMC()
	if(clientUMC<0){
		puts("Could not connect to the UMC. Please, try again.");
		return 0;
	}
	PCB_NEW_list = list_create();
	cpus_conectadas = list_create();
	setServerSocket(&consoleServer, setup.KERNEL_IP, setup.PUERTO_PROG);
	setServerSocket(&cpuServer, setup.KERNEL_IP, setup.PUERTO_CPU);
	printf(" .:: Servers to CPUs and consoles up and running ::.\n");
	printf(" .:: Waiting for incoming connections ::.\n\n");
	while(control_clients());
	puts("Bye bye kernel");//TODO Delete
	close(consoleServer);
	close(cpuServer);
	//close(clientUMC); // TODO un-comment when real UMC is present
	return 0;
}

int start_kernel(int argc, char* configFile){
	printf("\n\t=============================================\n");
	printf("\t.:: Vamo a calmarno que viene el Kernel ::.");
	printf("\n\t=============================================\n");
	if(argc==2){
		if(loadConfig(configFile)<0){
    		puts(" Config file can not be loaded.\n Please, try again.\n");
    		return -1;
    	}
	}else{
    	printf(" Usage: ./kernel setup.data \n");
    	return -1;
	}
	signal (SIGINT, tratarSeniales);
	signal (SIGPIPE, tratarSeniales);	
	return 0;
}

int loadConfig(char* configFile){
	if(configFile == NULL){
		return -1;
	}
	t_config *config = config_create(configFile);
	printf(" .:: Loading settings ::.\n");

	if(config != NULL){
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
	char* buffer=NULL;
	char* buffer_4=NULL;
	printf(" .:: Connecting to UMC on %s:%d ::.\n",setup.IP_UMC,setup.PUERTO_UMC);
	if(getClientSocket(&clientUMC, setup.IP_UMC, setup.PUERTO_UMC)) return (-1);
	sprintf(buffer_4, "%04d", (int) setup.STACK_SIZE);
	asprintf(&buffer, "%s%s", "0", buffer_4);
	send(clientUMC, buffer, 5 , 0);
	printf(" .:: Stack size (sent to UMC): %s ::.\n",buffer_4);
	if(recv(clientUMC, &buffer_4, 4, 0) < 0) return (-1);
	setup.PAGE_SIZE=atoi(buffer_4);
	printf(" .:: Page size: (received from UMC): %d ::.\n",setup.PAGE_SIZE);
	free(buffer);
	free(buffer_4);
	return clientUMC;
}

int	requestPages2UMC(char* PID, int ansisopLen,char* code,int clientUMC){
	/*
	This function MUST be in a thread
	Because the recv() is BLOCKER and it can be delayed when waiting
	for SWAP.
	Put all parameters in a serialized struct (create a stream);
	*/
	char* buffer;
	char buffer_4[4];
	int bufferLen=1+4+4+4+ansisopLen; //1+PID+req_pages+size+code
	sprintf(buffer_4, "%04d", (int) (ansisopLen/setup.PAGE_SIZE)+1);
	asprintf(&buffer, "%d%s%s%04d%s", 1,PID,buffer_4, ansisopLen,code);
	send(clientUMC, buffer, bufferLen, 0);
	recv(clientUMC, buffer_4, 4, 0);
	int code_pages=atoi(buffer_4);
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
int rmvClosedCPUs(){
	int i, j=0;
	if(cpusOnline == 0) return 0;
	for(i=0; i<cpusOnline; i++){
		if(cpuSockets[i]>0){
			cpuSockets[j] = cpuSockets[i];
			j++;
		}
	}
	cpusOnline = j; return cpuSockets[j-1];
}
int rmvClosedConsoles(){
	int i, j=0;
	if(consolesOnline == 0) return 0;
	for(i=0; i<consolesOnline; i++){
		if(consoleSockets[i]>0){	
			consoleSockets[j] = consoleSockets[i];
			j++;
		}
	}
	printf("Voy a devolver consoleSockets[j-1]=%d\n", consoleSockets[j-1]);
	consolesOnline = j; return consoleSockets[j-1];
}

void killCPU(int cpu){
	close(cpu);
	printf("Bye bye CPU %d\n", cpu);
	return;
}
void killCONSOLE(int console){
	close(console);
	printf("Bye bye Console %d\n", console);
	return;
}

int control_clients(){
	char buffer_4[4];
	buffer_4[0]='\0';
	int retval=-3;
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	int i;
	maxCPUs = rmvClosedCPUs();
	maxConsoles = rmvClosedConsoles();
	maxSocket=(maxCPUs>maxConsoles)?maxCPUs:maxConsoles;
	if(consoleServer>maxSocket) maxSocket=consoleServer;
	if(cpuServer>maxSocket) maxSocket=cpuServer;
	FD_ZERO(&allSockets);
	FD_ZERO(&checkClientCtrlC);
	FD_SET(cpuServer, &allSockets);
	FD_SET(consoleServer, &allSockets);
	for (i=0; i<cpusOnline; i++){
		FD_SET(cpuSockets[i], &allSockets);
		FD_SET(cpuSockets[i], &checkClientCtrlC);
	}
	for (i=0; i<consolesOnline; i++){
		FD_SET(consoleSockets[i], &allSockets);
		FD_SET(consoleSockets[i], &checkClientCtrlC);	
	}
	//retval=select(maxSocket+1, &allSockets, &checkClientCtrlC, NULL, &timeout);
	retval=select(maxSocket+1, &allSockets, NULL, NULL, &timeout);
	//printf("-----Acabo de salir del select() con %d-----\n",retval);
	//retval==0 -> no paso nada
	if(retval<0){
		perror("Un signal u otro error en el select()");
	}else if(retval>0){
		printf("Tengo %d consolas\n", consolesOnline);
		sleep(3);
		for(i=0;i<consolesOnline;i++){
			//if(FD_ISSET(consoleSockets[i], &checkClientCtrlC)){}else 
			if(FD_ISSET(consoleSockets[i], &allSockets)){
				// console has something to say
				printf("aca tampoco llego\n");
				if(recv(consoleSockets[i], buffer_4, 2, 0) > 0){
					printf("La consola no deberia decirme nada\n");
					if((strncmp(buffer_4, "T1",2)) == 0 ){
						write(consoleSockets[i], "holi", 4); //send fake PCB
					}else{
							printf("Caso no contemplado. Me dijeron:%s\n",buffer_4);
					}
				}else{
					printf(" .:: A console has closed the connection. PID %04d will be terminated ::. \n", consoleSockets[i]);
					killCONSOLE(consoleSockets[i]);
		        	consoleSockets[i] = -1;
		        	//return 2;
				//printf("Consola MURIOO! \n consolesOnline=%d, i=%d, consoleSockets[%d]=%d y n es-%d-\n",consolesOnline,i,i,consoleSockets[i], n);
				}
			}
		}
		for(i=0;i<cpusOnline;i++){
			/*if(FD_ISSET(cpuSockets[i], &checkClientCtrlC)){
				killCPU(cpuSockets[i]);
				cpuSockets[i] = -1;
			}else*/ if(FD_ISSET(cpuSockets[i], &allSockets)){
				// cpu has something to say
				printf("Aca una CPU hablandome y yo la ignoro.\n");
			}
		}
		/* ALWAYS enters here, accepts new connections and handshakes */
		/*
			desde el FD_ISSET para CPU y Consola hasta el primer strncmp
			que lee "0" podria estar todo en una funcion generica
		*/
		if(FD_ISSET(cpuServer, &allSockets)){
			int aceptado=acceptConnection(cpuServer);
			if (aceptado<1){
				perror("Accept failed :(");
			}else{
				cpuSockets[cpusOnline]=aceptado;
				cpusOnline++;
				if(recv(aceptado, buffer_4, 1, 0)>0){
					if(strncmp(buffer_4, "0",1) == 0){
						// Aca agrego la cpu a la lista
						// En vez de agregar solo el CPU ID podria
						// agregar algo copado como un struct
						// que tenga el cpu_status
						// las CPUs van en un queue!!!
						// cuando le mando un PCB hago un pop()
						// cuando me lo devuelve queda libre entonces hago
						// push() y la meto de nuevo en la cola
						list_add(cpus_conectadas,&aceptado);
						printf(" .:: New CPU arriving ::.\n");

						
						t_pcb *tuPCB;
						tuPCB=(list_get(PCB_NEW_list,0));
						tuPCB->status=EXECUTING;
						printf(" .:: Le mando a CPU el PCB %d ::.\n",tuPCB->pid);
						//serializo y mando
						send(aceptado,"pbc_serializado",5,0);
						//list_find(PCB_NEW_list, (void*) _is_chuck_norris);
					}
				}else{
					printf(" .:: CPU leaving ::.\n");
				}
			}
		}
		if(FD_ISSET(consoleServer, &allSockets)){
			int aceptado=acceptConnection(consoleServer);
			if(aceptado<1) perror("Accept failed :(");
			else{
				consoleSockets[consolesOnline]=aceptado;
				consolesOnline++;
				if(recv(aceptado, buffer_4, 1, 0)>0){
					if (strncmp(buffer_4, "0",1) == 0){
						/*
							Launch a thread here
							Because if a new console arrives and we request UMC
							pages to store the program code but SWAP is compacting,
							we have to wait... and we can't run RoundRobin while waiting.

							Consider making cpuSockets and consoleSockets, global variables
							because you don't want to serialize them to send them to the
							threads.
							And you don't want to put them into a struct.
						*/
						printf(" .:: New console arriving ::.\n");
						char PID[4];
						sprintf(PID, "%04d", (int) aceptado);
						recv(aceptado, buffer_4, 4, 0);
						int ansisopLen=atoi(buffer_4);
						char *code = malloc(ansisopLen);
						recv(aceptado, code, ansisopLen, 0);
						//printf("El codigo ansisop recibido es:\n%s\n",code);
						//int code_pages=requestPages2UMC(PID,ansisopLen,code,clientUMC);
						//TODO DELETEEE
						int code_pages=3;
						//END TODO deLETEEE
						if(code_pages>0){
							send(aceptado,PID,4,0);
							t_metadata_program* metadata = metadata_desde_literal(code);
							t_pcb newPCB;
							newPCB.pid=atoi(PID);
							newPCB.program_counter=metadata->instruccion_inicio;
							newPCB.stack_pointer=code_pages;
							newPCB.stack_index=queue_create();
							newPCB.status=NEW;
							newPCB.instrucciones_size=metadata->instrucciones_size;
							char * instrucciones_buffer = NULL;
	    					t_size instrucciones_buffer_size = 0;
	    					/*
	    					serialize_instrucciones(newMetadata->instrucciones_serializado, &instrucciones_buffer, &instrucciones_buffer_size) ;
	    					newPCB.instrucciones_serializado = instrucciones_buffer;
	    					*/
							//newPCB.instrucciones_serializado=metadata->instrucciones_serializado;
						//	newPCB.etiquetas_size=metadata->etiquetas_size;
						//	newPCB.etiquetas=metadata->etiquetas;

							list_add(PCB_NEW_list,&newPCB);
							//newPCB.status=EXECUTING;
							printf(" .:: The program with PID=%04d is now READY (%d) ::.\n", newPCB.pid, newPCB.status);
						}else{
							send(aceptado,"0000",4,0);
							printf(" .:: The program with PID=%s could not be started. System run out of memory ::.\n", PID);
							killCONSOLE(aceptado);
							consoleSockets[consolesOnline-1]=-1;
						}
						free(code); // let it free
					}else{
						printf(" .:: Consola dijo algo que no es handshake: %s\n", buffer_4);
					}
				}else{
					printf("Se fue una consola? Tenia %d consolas\n", consolesOnline);
					killCONSOLE(aceptado);
					consoleSockets[consolesOnline-1]=-1;
					printf("Me quedaron %d consolas\n", consolesOnline);
				}
			}
			printf("Consolas despues de aceptar: %d\n", consolesOnline);
		}
	}
	return 1;
}