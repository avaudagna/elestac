/* Kernel.c by pacevedo */
/*
Compilame asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o kernel socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c kernel.c -L/usr/lib -lcommons -lparser-ansisop
*/
#include <sys/time.h>
#include "kernel.h"
#include "libs/serialize.h"
#include "libs/pcb.h"
#include "libs/stack.h"



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
int main (int argc, char **argv) {
	PCB_NEW_list = list_create();	//TODO delete
	cpus_conectadas = list_create(); // use globales por todo lado 4 testing
	printf("\n\t=============================================\n");
	printf("\t.:: Vamo a calmarno que viene el Kernel ::.");
	printf("\n\t=============================================\n");
	if(argc==2){
		if(loadConfig(argv[1])<0){
    		puts(" Config file can not be loaded.\n Please, try again.\n");
    		return -1;
    	}
	}else{
    	printf(" Usage: ./kernel setup.data \n");
    	return -1;
	}
	//int clientUMC=connect2UMC();
	int clientUMC=100; //TODO Delete when connecting to the real UMC
	setup.PAGE_SIZE=1024; //TODO Delete -> lo hace connect2UMC()

	if(clientUMC<0){
		puts("Could not connect to the UMC. Please, try again.");
		return 0;
	}
	signal (SIGINT, tratarSeniales);
	
	setServerSocket(&consoleServer, setup.KERNEL_IP, setup.PUERTO_PROG);
	setServerSocket(&cpuServer, setup.KERNEL_IP, setup.PUERTO_CPU);
	printf(" .:: Servers to CPUs and consoles up and running ::.\n");
	printf(" .:: Waiting for incoming connections ::.\n\n");
	while(control_clients());
	close(consoleServer);
	close(cpuServer);
	//close(clientUMC);
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
	printf(" .:: Connecting to UMC on %s:%d ::.\n",setup.IP_UMC,setup.PUERTO_UMC);
	if(getClientSocket(&clientUMC, setup.IP_UMC, setup.PUERTO_UMC)) return (-1);
	sprintf(global_buffer_4, "%04d", (int) setup.STACK_SIZE);
	char* buffer;
	asprintf(&buffer, "%s%s", "0", global_buffer_4);
	send(clientUMC, buffer, 5 , 0);
	printf(" .:: Stack size (sent to UMC): %s ::.\n",global_buffer_4);
	if(recv(clientUMC, &global_buffer_4, 4, 0) < 0) return (-1);
	setup.PAGE_SIZE=atoi(global_buffer_4);
	printf(" .:: Page size: (received from UMC): %d ::.\n",setup.PAGE_SIZE);
	free(buffer);
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
	int bufferLen=1+4+4+4+ansisopLen; //1+PID+req_pages+size+code
	sprintf(global_buffer_4, "%04d", (int) (ansisopLen/setup.PAGE_SIZE)+1);
	asprintf(&buffer, "%d%s%s%04d%s", 1,PID,global_buffer_4, ansisopLen,code);
	send(clientUMC, buffer, bufferLen, 0);
	recv(clientUMC, global_buffer_4, 4, 0);
	int code_pages=atoi(global_buffer_4);
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
	}
}
int rmvClosedCPUs(int *cpuList, int *cpusOnline){
	int i;int j=0;
	if(*cpusOnline == 0) return 0;
	for(i=0; i<(*cpusOnline); i++){
		if (cpuList[i]==-1)	killCPU(cpuList[i]);
		if(cpuList[i]>0){
			cpuList[j] = cpuList[i];
			j++;
		}
	}
	*cpusOnline = j; return cpuList[j];
}
int rmvClosedConsoles(int *consoleList, int *consolesOnline){
	int i;int j=0;
	if(*consolesOnline == 0) return 0;
	for(i=0; i<(*consolesOnline); i++){
		if(consoleList[i] == -1) killCONSOLE(consoleList[i]);
		if(consoleList[i]>0){	
			consoleList[j] = consoleList[i];
			j++;
		}
	}
	*consolesOnline = j; return consoleList[j];
}
void killCPU(int cpu){
	printf("Bye bye CPU %d\n", cpu);
}
void killCONSOLE(int console){
	printf("Bye bye Console %d\n", console);
}

int control_clients(){
	int retval;
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	int i;
	fd_set 	allSockets,
			checkClientCtrlC;

	sleep(1); // TODO Delete
	maxCPUs = rmvClosedCPUs (cpuSockets, &cpusOnline); /* update cpus counter */
	maxConsoles = rmvClosedConsoles (consoleSockets, &consolesOnline); /* update consoles counter */
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
	retval=select(maxSocket+1, &allSockets, &checkClientCtrlC, NULL, &timeout);
	printf("-------------Acabo de salir del select() con %d---------\n",retval);
	for(i=0;i<consolesOnline;i++){
		if(FD_ISSET(consoleSockets[i], &checkClientCtrlC)){
			printf(" .:: A console has closed the connection. PID %04d will be terminated ::. \n", consoleSockets[i]);
			killCONSOLE(consoleSockets[i]);
	        consoleSockets[i] = -1;
	        close(consoleSockets[i]);
		}else if(FD_ISSET(consoleSockets[i], &allSockets)){
			// console has something to say
			if(recv(consoleSockets[i], global_buffer_4, 2, 0) > 0){
				printf("La consola no deberia decirme nada\n");
				if((strcmp(global_buffer_4, "C1")) == 0 ){
					// CPU msg 1
					//write(clientSocket, fakePCB, strlen(fakePCB)); //send fake PCB
					write(consoleSockets[i], "holi", 4); //send fake PCB
					while(recv(consoleSockets[i], global_buffer_4 , 2 , 0) > 0){
						// wait for CPU to finish
						if(global_buffer_4!=NULL) puts(global_buffer_4);
						break;
					}
				}else{
						printf("Caso no contemplado. Me dijeron:%s\n",global_buffer_4);
				}
			}else{
				printf("Ocurrió un error en el recv de una consola.\n");
			}
		}
	}
	for(i=0;i<cpusOnline;i++){
		if(FD_ISSET(cpuSockets[i], &checkClientCtrlC)){
			killCPU(cpuSockets[i]);
		}else if(FD_ISSET(cpuSockets[i], &allSockets)){
			// cpu has something to say
			printf("Aca una CPU hablandome y yo la ignoro.\n");
		}
	}
	/* ALWAYS enters here, accepts new connections and handshakes */
	/*
		desde el FD_ISSET para CPU y Consola hasta el primer strcmp
		que lee "0" podria estar todo en una funcion generica
	*/
	if (FD_ISSET(cpuServer, &allSockets)){
		int aceptado=acceptConnection(cpuServer);
		if (aceptado<1){
			perror("Accept failed :(");
		}else{
			cpuSockets[cpusOnline]=aceptado;
			cpusOnline++;
			if(recv(aceptado, &global_buffer_4, 1, 0)>0){
				if(strcmp(global_buffer_4, "0") == 0){
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
					send(aceptado,"puto",5,0);
					//list_find(PCB_NEW_list, (void*) _is_chuck_norris);
				}
			}else{
				printf(" .:: CPU leaving ::.\n");
			}
		}
	}
	if (FD_ISSET(consoleServer, &allSockets)){
		int aceptado=acceptConnection(consoleServer);
		if (aceptado<1){
			perror("Accept failed :(");
		}else{
			consoleSockets[consolesOnline]=aceptado;
			consolesOnline++;
			if(recv(aceptado, &global_buffer_4, 1, 0)>0){
				if (strcmp(global_buffer_4, "0") == 0){
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
					recv(aceptado, &global_buffer_4, 4, 0);
					int ansisopLen=atoi(global_buffer_4);
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
						//newPCB.instrucciones_serializado=metadata->instrucciones_serializado;
					//	newPCB.etiquetas_size=metadata->etiquetas_size;
					//	newPCB.etiquetas=metadata->etiquetas;

						list_add(PCB_NEW_list,&newPCB);
						//newPCB.status=EXECUTING;
						printf(" .:: The program with PID=%04d is now READY (%d) ::.\n", newPCB.pid, newPCB.status);
					}else{
						send(aceptado,"0000",4,0);
						printf(" .:: The program with PID=%s could not be started. System run out of memory ::.\n", PID);
						close(aceptado);
						consolesOnline--;
						consoleSockets[consolesOnline]=-1;
					}
					free(code); // let it free
				}else{
					printf(" .:: Consola dijo algo que no es handshake: %s\n", global_buffer_4);
				}
			}else{
				close(aceptado);
				consolesOnline--;
				consoleSockets[consolesOnline]=-1;
			}
		}
		printf("Consolas despues de aceptar: %d\n", consolesOnline);
	}
	return 1;
}