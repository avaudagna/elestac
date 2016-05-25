/* Kernel.c by pacevedo */
/*
Compilame asi:
gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o kernel socketCommons.c kernel.c -L/usr/lib -lcommons -lparser-ansisop
*/
#include "kernel.h"
int main (int argc, char **argv) {
	puts(" .:: Vamo a calmarno que viene el Kernel ::.");
	if(argc==2){
		if(loadConfig(argv[1])<0){
    		puts(" Config file can not be loaded.\n Please, try again.\n");
    		return -1;
    	}
	}else{
    	printf(" Usage: ./kernel setup.data \n\n");
    	return -1;
	}
	//int clientUMC=connect2UMC();
	int clientUMC=100;
	setup.PAGE_SIZE=1024;
	if(clientUMC<0){
		puts("Could not connect to the UMC. Please, try again.");
		return 0;
	}
	signal (SIGINT, tratarSeniales);
	int i;
	fd_set allSockets;

	int serverSocket;
	int read_size = 0;
	int consoleSockets[MAX_CLIENTS]={0};
	int cpuSockets[MAX_CLIENTS]={0};
	int cpusOnline=0;
	int consolesOnline=0;
	int maxSocket=0;
	int maxCPUs=0;
	int maxConsoles=0;

		int clientSocket[MAX_CLIENTS]={0}; // TODO Delete
		int clientsOnline=0; // TODO Delete


	t_list* kernel_cpus_conectadas = list_create();
	t_list* kernel_consolas_conectadas = list_create();

	char *messageBuffer = (char*) malloc(sizeof(char)*1024); // TODO delete
	if(messageBuffer == NULL) return (-1);

	setServerSocket(&serverSocket, KERNEL_IP, KERNEL_PORT);
	printf(" .:: Server created ::.\n\n");
	while(1){
		sleep(1); // TODO Delete
		maxCPUs = rmvClosedCPUs (cpuSockets, &cpusOnline); /* update clients online counter */
		maxConsoles = rmvClosedConsoles (consoleSockets, &consolesOnline); /* update clients online counter */
     	maxSocket=(maxCPUs>maxConsoles)?maxCPUs:maxConsoles;
		if(maxSocket<serverSocket) maxSocket=serverSocket;
		FD_ZERO(&allSockets); /* Clear all sockets */
		FD_SET(serverSocket, &allSockets);
		/* Add all clients to the select() */
		for (i=0; i<clientsOnline; i++)	FD_SET(clientSocket[i], &allSockets);
		select (maxSocket+1, &allSockets, NULL, NULL, NULL);
		/* Check existing clients */
		for (i=0; i<clientsOnline; i++){
			if(FD_ISSET(clientSocket[i], &allSockets)){
				if((read_size = recv(clientSocket[i], messageBuffer, 2, 0)) > 0){
					if(strcmp(messageBuffer, "C0") == 0){
						//CPU connection
						read_size = recv(clientSocket[i], messageBuffer, 2, 0);
						printf("CPU says:%s\n",messageBuffer);
					}else if((strcmp(messageBuffer, "C1")) == 0 ){
						// CPU msg 1
						//write(clientSocket, fakePCB, strlen(fakePCB)); //send fake PCB
						write(clientSocket[i], "holi", 4); //send fake PCB
						while( (read_size = recv(clientSocket[i], messageBuffer , 2 , 0)) > 0 ) {
							// wait for CPU to finish
							if(messageBuffer!=NULL) puts(messageBuffer);
							break;
						}
					}else{
							puts("Caso no contemplado. Caimos en default (?");
							printf("Los clientes andan diciendo:%s\n",messageBuffer);
					}
				}else{
		            /* Error in rcv, possible connection lost.
		             * Close the socket and delete the client. */
		        	printf("Client %d closed the connection. \n", i+1);
		        	fflush(stdout);
		        	//FD_CLR(clientSocket[i], &allSockets);
		        	clientSocket[i] = -1;
		        	close(clientSocket[i]);
	/*
	 * TODO
	 *  Acá tengo que ver si es la consola (que se mandó un ctrl+c o similar)
	 *  Si es la consola (la tengo identificada en un vector paralelo por ID),
	 *  Tengo que avisarle a CPU y UMC que ya fue, que cierre todo y nos vamo a casa.*/
		        }
		    }
		}
		/* Accept new connections and handshake */
		if (FD_ISSET(serverSocket, &allSockets)){
			// ALWAYS enters here and checks for new clients
			// Function will return 0 or -1 unless a new connection was created!
			printf("Clientes antes de aceptar: %d \n", clientsOnline);
			int aceptado=newClient(serverSocket, clientSocket, clientsOnline);
			if(aceptado>clientsOnline){
				clientsOnline=aceptado;
				char clientID[2];
				sprintf(clientID, "%02d", (int) clientSocket[clientsOnline-1]);
				if((read_size = recv(clientSocket[clientsOnline-1], messageBuffer, 2, 0)) > 0){
					if(strcmp(messageBuffer, "T0") == 0){
						puts ("A console is calling, let's handshake!");
						// TODO Add the console to the list of console clients.
						/* Server sends to the client the clientID */
						write(clientSocket[clientsOnline-1],clientID,3);
						char progSize[4];
						recv(clientSocket[clientsOnline-1], messageBuffer, 2, 0);//Leo T1 -> TODO eliminar esto del protocolo porque molesta
						recv(clientSocket[clientsOnline-1], progSize, 4, 0); // Leo el size del programa
						int ansisopLen=atoi(progSize);
						char *codigo = malloc(ansisopLen);
						recv(clientSocket[clientsOnline-1], codigo, ansisopLen, 0);
						t_metadata_program* newPCB = metadata_desde_literal(codigo);
						printf("El codigo ansisop recibido es:\n%s\n",codigo);
						int instrucciones=newPCB->instrucciones_size;
						puts("El cod");
						for(i=0;i<instrucciones;i++){
							(newPCB->instrucciones_serializado+i)->start;
							(newPCB->instrucciones_serializado+i)->offset;
						}
						//printf("Cantidad de etiquetas: %i \n\n",newPCB->cantidad_de_etiquetas);
						if(requestPages2UMC(newPCB)){
							puts("Ask UMC for pages to allocate PCB");
							//create PCB and submit to UMC for storage
							//serialize newPCB and submit
						}else{
							puts("Sorry, no hay espacio para tu programita.");
						}
						free(codigo); // let it free
					}
				}
			}
			printf("Clientes despues de aceptar: %d\n", clientsOnline);
		}
	}
	free(messageBuffer);
	close(serverSocket);
	return 0;
}

int loadConfig(char* configFile){
	if(configFile == NULL){
		return -1;
	}
	t_config *config = config_create(configFile);
	puts(" .:: Loading settings ::.");

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
	return clientUMC;
}

int requestPages2UMC(t_metadata_program* metadata){
	int clientUMC=5; //TODO Delete
	char* msg = (char*) calloc(sizeof(char),2);
	send(clientUMC , "hay espacio?" , 2 , 0);
	if( recv(clientUMC , msg , 1, 0) < 0) {
	   puts("recv failed");
	}
	if( send(clientUMC , "codigo", 2 , 0) < 0) {
	   puts("Send failed");
	   return 1;
	}
	if( recv(clientUMC , msg , 2 , 0) < 0) {
		puts("recv failed");
	}
	puts("UMC reply :");
	puts(msg);
	close(clientUMC);
	return 1;
}
void tratarSeniales(int senial){
	printf ("Tratando seniales\n");
	printf("\nSenial: %d\n",senial);
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

int newClient(int serverSocket, int *clientSocket, int clientsOnline){
/* Accepts a new connection from a client.
 * Returns  0 if max clients have been reached.
 * Returns -1 if the connection failed.
 * Returns the socket descriptor ID if success was achieved.
 */
//	if(clientsOnline>=MAX_CLIENTS) return 0;
	clientSocket[clientsOnline]=acceptConnection((int) clientSocket[clientsOnline],serverSocket);
	if (clientSocket[clientsOnline] < 0) {
		perror("Accept failed :(");
		return -1;
	}else{
		printf ("Client number %d accepted! :)\n", clientSocket[clientsOnline]);
	}
	return (++clientsOnline);
}