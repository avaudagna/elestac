#ifndef CPU_CPU_INIT_H
#define CPU_CPU_INIT_H

#include <stdio.h>
#include "libs/socketCommons.h"
#include "libs/t_setup.h"
#include <signal.h>
#include <commons/log.h>
#include <commons/config.h>

#define SUCCESS 1
#define ERROR -1

extern int umcSocketClient;
extern int kernelSocketClient;
extern t_setup setup;
extern t_log* cpu_log;

int cpu_init(int argc, char *config_file);

int setup_config_init(char* config_file);
int usage_check(int argc);
void print_cpu_banner();
int client_sockets_init();
void signalsInit();
int loadConfig(char* configFile);
void tratarSeniales(int senial);

#endif //CPU_CPU_INIT_H
