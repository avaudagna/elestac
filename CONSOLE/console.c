#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <socket-commons/socketCommons.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define KERNEL_ADDR "127.0.0.1"
#define KERNEL_PORT 54326

#define PACKAGE_SIZE 10


void tratarSeniales(int);
int main(int argc, char *argv[]) {

	char pid_console[4];
	char path[1024];
	if (getcwd(path, sizeof(path)) != NULL) {
		fprintf(stdout, "Current working dir: %s\n", path);
	} else {
		perror("getcwd() error");
	}

	strcat(path, "/console.config");

	FILE * config;
	config = fopen(path, "r");
	if (!config) {
		printf("Failed to open config file\n");
		exit(1);
	}

	signal(SIGINT, tratarSeniales);
	int kernelSocketClient;
	char kernel_reply[2000];
	//create kernel client
	if (argc != 2) {
		puts("usage: console file");
		return -1;
	}
	getClientSocket(&kernelSocketClient, KERNEL_ADDR, KERNEL_PORT);

	/*le voy a mandar al kernel una T0 (terminal),
	 y me va a devolver el client_id (un número) que me represente con el cual él me conoce
	 Luego T1+ sizeMsj en 4B +(el código como viene)*/
	//receive a reply from the kernel
	send(kernelSocketClient, "T0", PACKAGE_SIZE, 0);

	do {
		if (recv(kernelSocketClient, kernel_reply, 4, 0) < 0) {
			puts("recv failed");
			break;
		}
		puts("Kernel reply :");
		puts(kernel_reply);
		*pid_console = kernel_reply;


	}
	/*
	 * OJO con este while, ya no recibo "kernel"...
	 */
	while (strcmp(kernel_reply, "kernel"));

	// le envío a kernel T1+sizeMsj en 4B+mensaje

	FILE * fp;
	fp = fopen(fp, "r");
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

	int sizeMsj = strlen("T1") + 5 + (int) sz; //T1 no se lo voy a enviar más... sacar
	char* mensaje = (char*) malloc(sizeMsj);

	char buffer[20];
	sprintf(buffer, "%04d", (int) sz);

	strcpy(mensaje, "T1");
	strcat(mensaje, buffer);
	strcat(mensaje, prog);
	send(kernelSocketClient, mensaje, sizeMsj, 0);

	do {
		if (recv(kernelSocketClient, kernel_reply, 2, 0) < 0) {
			puts("recv failed");
			break;
		}
		recv(kernelSocketClient, kernel_reply, 2, 0);
		if (!strcmp(kernel_reply, "ID")) {
			//recibo luego 5 -> 1 variable + 4 valor_variable
			char var[1];
			char valor[4];
			recv(kernelSocketClient, var, 1, 0);
			recv(kernelSocketClient, valor, 4, 0);
			printf("El valor de la variable %s es: %s", var, valor);//controlar este printf

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

	close(kernelSocketClient);
	puts("Terminated console: ");
	puts(pid_console);
	return 0;

}

int loadConfig(char* configFile){
	if(configFile == NULL){
		return -1;
	}
	t_config *config = config_create(configFile);
	puts(" .:: Loading settings ::.");

	if(config != NULL){
		setup.KERNEL_PORT=config_get_int_value(config,"PUERTO_KERNEL");
		setup.KERNEL_ADDR=config_get_string_value(config,"IP_KERNEL");
	}
	//config_destroy(config);
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

/*
 1) ./ejemplo.ansisop(porAhoraHola)
 2) consola ("consola") -> kernel ("kernell") -> consola (porAhoraHola) -> kernel
 3) kernel -> cpu
 4)kernel (porAhoraHola) -> umc (porAhoraHola NO) -> kernel (porAhoraHola NO) -> consola
 5) CPU (porAhoraHola) -> UMC
 6) UMC (porAhoraHola) -> swap
 */

