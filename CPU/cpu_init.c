#include "cpu_init.h"

int log_init();

int setup_config_init(char* config_file) {
    if(loadConfig(config_file)<0) {
        printf("Config file could not be loaded.\n Please, try again.\n");
        return ERROR;
    }
}

int cpu_init(int argc, char *config_file) {
    print_cpu_banner();
//    mallopt(M_CHECK_ACTION, 3);
    signalsInit();

    return (usage_check(argc) && log_init() &&
            setup_config_init(config_file) && client_sockets_init() );
}

int log_init() {

    cpu_log = log_create("cpu.log", "Elestac-CPU", true, LOG_LEVEL_TRACE);
    if(cpu_log == NULL) {
        printf("Could not create log\n");
        return ERROR;
    }
    else {
        return SUCCESS;
    }
}

int usage_check(int argc) {
    if(argc!=2) {
        printf("Usage: ./cpu setup->data \n");
        return ERROR;
    }
    return SUCCESS;
}

void print_cpu_banner() {
    printf("\n\t===========================================================\n");
    printf("\t.:: Me llama usted, entonces voy, el CPU es quien yo soy ::.");
    printf("\n\t===========================================================\n\n");
}

int client_sockets_init() {
    //Crete UMC client socket
    if (getClientSocket(&umcSocketClient, setup->IP_UMC , setup->PUERTO_UMC) == 0) {
        log_info(cpu_log,"New UMC socket obtained");
    } else {
        log_error(cpu_log, "Could not obtain UMC socket");
        return ERROR;
    }
    //Crete Kernel client socket
    if (getClientSocket(&kernelSocketClient, setup->KERNEL_IP, setup->PUERTO_KERNEL) == 0) {
        log_info(cpu_log,"New KERNEL socket obtained");
    }
    else {
        log_error(cpu_log, "Could not obtain KERNEL socket");
        return ERROR;
    }
    return SUCCESS;
}

void signalsInit() {
    signal (SIGINT, tratarSeniales);
    signal (SIGPIPE, tratarSeniales);
    signal (SIGUSR1, tratarSeniales);
}

int loadConfig(char* configFile){
    if(configFile == NULL){
        return ERROR;
    }
    t_config *config = config_create(configFile);
    printf(" .:: Loading settings ::.\n");

    setup = (t_setup*) malloc(sizeof(t_setup));
    if(config != NULL){
        setup->PUERTO_KERNEL=config_get_int_value(config,"PUERTO_KERNEL");
        setup->KERNEL_IP=config_get_string_value(config,"KERNEL_IP");
        //setup->IO_ID=config_get_array_value(config,"IO_ID");
        //setup->IO_SLEEP=config_get_array_value(config,"IO_SLEEP");
  //      setup->SEM_ID=config_get_array_value(config,"SEM_ID");
 //       setup->SEM_INIT=config_get_array_value(config,"SEM_INIT");
//      setup->SHARED_VARS=config_get_array_value(config,"SHARED_VARS");
        setup->PUERTO_UMC=config_get_int_value(config,"PUERTO_UMC");
        setup->IP_UMC=config_get_string_value(config,"IP_UMC");
    }
    //config_destroy(config);
    return SUCCESS;
}

void tratarSeniales(int senial){
    printf("\n\t=============================================\n");
    printf("\t\tSystem received the signal: %d",senial);
    printf("\n\t=============================================\n");
    switch (senial){
        case SIGINT:
            // Detecta Ctrl+C y evita el cierre.
            printf("This will end the CPU. Press Ctrl+C again to confirm.\n\n");
            signal (SIGINT, SIG_DFL); // solo controlo una vez.
            break;
        case SIGPIPE:
            // Trato de escribir en un socket que cerro la conexion.
            printf("The KERNEL or UMC connection droped down.\n\n");
            signal (SIGPIPE, tratarSeniales);
            break;
        case SIGUSR1:
            printf("SIGUSR1 signal received\n");
            printf("At the end of the Quantum, this CPU will disconnect\n");
            LAST_QUANTUM_FLAG = 1;
            signal (SIGUSR1, tratarSeniales);
            break;
        default:
            printf("Other signal received\n");
            break;
    }
}