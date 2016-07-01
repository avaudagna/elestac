#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include "socketCommons/socketCommons.h"
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <commons/config.h>
#include <commons/log.h>

struct {
		int 	PUERTO_KERNEL;
		char*	IP_KERNEL;
	} setup;


void tratarSeniales(int);
int loadConfig(char* configFile);
t_log *console_log;

int main(int argc, char *argv[]) {
	signal(SIGINT, tratarSeniales);
	console_log = log_create("console.log", "Elestac-CONSOLE", true, LOG_LEVEL_TRACE);

	if (loadConfig("/usr/share/ansisop/console.config") < 0){
		log_error(console_log, "No se encontró el archivo de configuración");
		return -1;
	}

	int kernelSocketClient;
	char *kernel_reply = malloc(4);
	//create kernel client
	if (argc != 2) {
		puts("usage: console file");
		return -1;
	}

	int retorno = getClientSocket(&kernelSocketClient, setup.IP_KERNEL, setup.PUERTO_KERNEL);

	log_info(console_log, "Se cargó el setup con IP y puerto del kernel");
	/*le voy a mandar al kernel un 0 para iniciar el handshake + sizeMsj en 4B + (el código como viene),
	y me va a devolver el client_id (un número) que me represente con el cual él me conoce*/

	FILE *fp;
	fp = fopen(argv[1], "r");
	if (!fp) {
		log_error(console_log, "No se puedo abrir el ansisop.");
		exit(1);
	}

	fseek(fp, SEEK_SET, 0);
	fseek(fp, 0L, SEEK_END);
	long int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char hash_bang[100];
	fgets(hash_bang, 100, fp);
	sz = sz - strlen(hash_bang) - 1;

	char *prog = (char *) malloc(sz);

	fread(prog, sz + 1, 1, fp);
	fclose(fp);

	int sizeMsj = strlen("0") + 5 + (int) sz;
	char *mensaje = (char *) malloc(sizeMsj);

	char buffer[20];
	sprintf(buffer, "%04d", (int) sz);

	strcpy(mensaje, "0");
	strcat(mensaje, buffer);
	strcat(mensaje, prog);
	send(kernelSocketClient, mensaje, sizeMsj, 0);
	log_info(console_log, "El tamanio del programa es %s, y se envio al kernel: %s", buffer, mensaje);

	free(prog);
	free(mensaje);

	if (recv(kernelSocketClient, kernel_reply, 4, 0) < 0) {
		log_error(console_log, "Kernel no responde");
		return EXIT_FAILURE;
	} else if (!strcmp(kernel_reply, "0000")) {
		log_error(console_log, "Kernel contesto: %s.No hay espacio en memoria para ejecutar el programa", kernel_reply);
	} else {
		log_info(console_log, "El pid de la consola es %s. Se esta ejecutando el programa.", kernel_reply);
	}

	char *valor = malloc(4);
	char textSize[4];
	int textLen;
	bool continua = true;

	while (continua) {
		if (recv(kernelSocketClient, kernel_reply, 1, 0) > 0) {
			log_info(console_log, "Kernel dijo: %s . Ejecutar protocolo correspondiente", kernel_reply);
			switch (atoi(kernel_reply)) {
				case 0:// Program END
					continua = false;
					log_info(console_log, "Vamo a recontra calmarno. El programa finalizó correctamente");
					break;
				case 1:// Print value
					//recibo 8 bytes -> 4 sizeof(variable) + variable + 4 valor_variable

					recv(kernelSocketClient, textSize, 4, 0);
					textLen = atoi(textSize);
					char *var = malloc(textLen);

					recv(kernelSocketClient, var, textLen, 0);
					recv(kernelSocketClient, valor, 4, 0);
					log_info(console_log, "El valor de la variable %s es: %s", var, valor);
					free(var);
					break;

				case 2:// Print text
					//recibo 4 del tamaño + texto

					recv(kernelSocketClient, textSize, 4, 0);
					textLen = atoi(textSize);
					recv(kernelSocketClient, kernel_reply, textLen, 0);
					log_info(console_log, "%s.", kernel_reply);
					break;
			}
		}
	}

	free(valor);
	free(kernel_reply);
	close(kernelSocketClient);
	log_info(console_log, "Se cerro la conexion con el kernel");
	puts("Terminated console.");
	log_destroy(console_log);
	return 0;

}

int loadConfig(char* configFile){
	if(configFile == NULL){
		return -1;
	}
	t_config *config = config_create(configFile);
	log_info(console_log, " .:: Loading settings ::.");

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

