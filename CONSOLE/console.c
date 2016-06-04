#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <socket-commons/socketCommons.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <commons/config.h>

struct {
		int 	PUERTO_KERNEL;
		char*	IP_KERNEL;
	} setup;


void tratarSeniales(int);
int loadConfig(char* configFile);

int main(int argc, char *argv[]) {
	signal(SIGINT, tratarSeniales);
	char path[1024];

	if (getcwd(path, sizeof(path)) != NULL) {
		fprintf(stdout, "Current working dir: %s\n", path);
	} else {
		perror("getcwd() error");
	}

	strcat(path, "/console.config");

	if (loadConfig(path)<0) return -1;

	int kernelSocketClient;
	char *kernel_reply = malloc(4);
	//create kernel client
	if (argc != 2) {
		puts("usage: console file");
		return -1;
	}

	getClientSocket(&kernelSocketClient, setup.IP_KERNEL, setup.PUERTO_KERNEL);

	printf("se cargó el setup\n");
	/*le voy a mandar al kernel un 0 para iniciar el handshake + sizeMsj en 4B + (el código como viene),
	 y me va a devolver el client_id (un número) que me represente con el cual él me conoce*/

	FILE * fp;
	fp = fopen(argv[1], "r");
	if (!fp) {
		printf("Failed to open text file\n");
		exit(1);
	}

	fseek(fp, SEEK_SET, 0);

	fseek(fp, 0L, SEEK_END);
	long int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char* prog = (char*) malloc(sz);

	fread(prog, sz + 1, 1, fp);
	fclose(fp);

	int sizeMsj = strlen("0") + 5 + (int) sz; //T1 no se lo voy a enviar más... sacar
	char* mensaje = (char*) malloc(sizeMsj);

	char buffer[20];
	sprintf(buffer, "%04d", (int) sz);

	strcpy(mensaje, "0");
	strcat(mensaje, buffer);
	strcat(mensaje, prog);
	send(kernelSocketClient, mensaje, sizeMsj, 0);

	do {
		if (recv(kernelSocketClient, kernel_reply, 4, 0) < 0) {
			puts("recv failed");
			break;
		} else if(!strcmp(kernel_reply, "0000")){
			printf("No hay suficiente espacio en memoria para ejecutar el programa\n");
		} else{
			printf("Vamo a calmarno. Su programa se está ejecutando\n");
			puts("Kernel reply :");
			puts(kernel_reply);
		}

	}
	/*
	 * OJO con este while, ya no recibo "kernel"...
	 */
	while (1);

	do {
		if (recv(kernelSocketClient, kernel_reply, 2, 0) < 0) {
			puts("recv failed");
			break;
		}
		recv(kernelSocketClient, kernel_reply, 2, 0);
		if (!strcmp(kernel_reply, "ID")) {
			//recibo luego 5 -> 1 variable + 4 valor_variable
			char *var = malloc(1);
			char *valor = malloc(4);
			recv(kernelSocketClient, var, 1, 0);
			recv(kernelSocketClient, valor, 4, 0);
			printf("El valor de la variable %s es: %s", var, valor);//controlar este printf
			free(var);
			free(valor);

		} else if(!strcmp(kernel_reply, "IT")) {
			//recibo 4 del tamaño + texto
			char textSize[4];

			recv(kernelSocketClient, textSize, 4, 0);
			int textLen=atoi(textSize);
			recv(kernelSocketClient, kernel_reply, textLen, 0);
			printf("%s\n", kernel_reply);//controlar este printf
		}

	}
	while(1);

	free(kernel_reply);
	close(kernelSocketClient);
	puts("Terminated console.");
	return 0;

}

int loadConfig(char* configFile){
	if(configFile == NULL){
		return -1;
	}
	t_config *config = config_create(configFile);
	puts(" .:: Loading settings ::.");

	if(config != NULL){
		setup.PUERTO_KERNEL=config_get_int_value(config,"PUERTO_KERNEL");
		setup.IP_KERNEL=config_get_string_value(config,"IP_KERNEL");
	}
	return 0;
}

void tratarSeniales(int senial) {
	printf("Tratando seniales\n");
	printf("\nSenial: %d\n", senial);
	switch (senial) {
	case SIGINT:
		// Detecta Ctrl+C y evita el cierre.
		printf(
				"Esto acabará con el sistema. Presione Ctrl+C una vez más para confirmar.\n\n");
		signal(SIGINT, SIG_DFL); // solo controlo una vez.
		break;
	}

}

