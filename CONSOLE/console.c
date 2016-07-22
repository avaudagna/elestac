#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include "libs/socketCommons.h"
#include <signal.h>
#include <commons/config.h>
#include <commons/log.h>
#include "libs/serialize.h"

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
	char *kernel_reply = calloc(1, sizeof(int));
	//create kernel client
	if (argc != 2) {
		puts("usage: console ansisopFile");
		return -1;
	}

	getClientSocket(&kernelSocketClient, setup.IP_KERNEL, setup.PUERTO_KERNEL);

	log_info(console_log, "Se cargó el setup con IP %s y puerto %d de KERNEL", setup.IP_KERNEL, setup.PUERTO_KERNEL);
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

	char *prog = (char *) calloc(1, sz);

	fread(prog, sz + 1, 1, fp);
	fclose(fp);

	//int sizeMsj = 1 + 4 + (int) sz; // 1+ansisop_len+ansisop
	void *mensaje = NULL;
	int mensaje_index = 0;
	char kernelProtocol = '0';

	serialize_data(&kernelProtocol, (size_t) sizeof(char), &mensaje, &mensaje_index);
	serialize_data(&sz, (size_t) sizeof(int), &mensaje, &mensaje_index);
	serialize_data(prog, (size_t) sz, &mensaje, &mensaje_index);
	send(kernelSocketClient, mensaje, (size_t) mensaje_index, 0);
	printf("El tamanio del programa es %d, y se envio al kernel: %s", (size_t) sz, (char *) mensaje);

	free(prog);
	//verificar si este es necesario:
	free(mensaje);

	if (recv(kernelSocketClient, kernel_reply, sizeof(int), 0) < 0) {
		log_error(console_log, "Kernel no responde");
		return EXIT_FAILURE;
	} else {
		int pid_console = 0;
		int buffindex = 0;
		deserialize_data(&pid_console, sizeof(int), kernel_reply, &buffindex);
		if (pid_console == 0) {
			log_error(console_log, "Kernel contesto: %d. No hay espacio en memoria para ejecutar el programa",
					  pid_console);
		} else {
			log_info(console_log, "El pid de la consola es %d. Se esta ejecutando el programa.", pid_console);
		}
	}
	free(kernel_reply);

	void *kernelBuffer = NULL;
	int valor = 0;
	int textLen = 0;
	bool continua = true;
	void *kernel_operation =  calloc(1, sizeof(int));
	int buffindex;
	while (continua) {
		buffindex = 0;
		if (recv(kernelSocketClient, kernel_operation, sizeof(int), 0) > 0) {
			int kernel_operation_index = 0;
			int operation;
			deserialize_data(&operation, sizeof(int), kernel_operation, &kernel_operation_index);
			log_info(console_log, "Kernel dijo: %d", operation);
			switch (operation) {
				case 0:// Program END
					continua = false;
					log_info(console_log, "Vamo a recontra calmarno. El programa finalizó correctamente");
					break;
				case 1:// Print value
					//recibo 4 bytes -> valor_variable
					kernelBuffer = calloc(1,sizeof(int));
					recv(kernelSocketClient, kernelBuffer, sizeof(int), 0);
					//valor = 0;
					//buffindex=0;
					deserialize_data(&valor, sizeof(int), kernelBuffer, &buffindex);
					log_info(console_log, "El valor de la variable es: %d", valor);
					free(kernelBuffer);
					break;
				case 2:// Print text
					//recibo 4 del tamaño + texto
					kernelBuffer = calloc(1,sizeof(int));
					recv(kernelSocketClient, kernelBuffer, sizeof(int), 0);
					//textLen = 0;
					//buffindex=0;
					deserialize_data(&textLen, sizeof(int), kernelBuffer, &buffindex);
					free(kernelBuffer);
					if(textLen > 0) {
						char* text = calloc(1, (size_t) textLen);
						recv(kernelSocketClient, text, (size_t) textLen, 0);
						log_info(console_log, "%s.", text);
						free(text);
					} else {
						log_error(console_log, "Se recibio un tamanio de texto invalido");
					}
					//free(kernelBuffer);
					break;
				case 3:
					continua = false;
					log_error(console_log, "You are a raper. There is a segmentation fault");
					break;
				default:
					continua = false;
					log_error(console_log, "Se registro un error. El programa finalizo catastroficamente");
					break;
			}
		} else {
			continua = false;
			log_error(console_log, "El programa finalizo repentinamente");
		}
	}

	free(kernel_operation);
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
			printf("Esto acabará con el sistema. Presione Ctrl+C una vez más para confirmar.\n\n");
			signal(SIGINT, SIG_DFL); // solo controlo una vez.
			break;
	}
}