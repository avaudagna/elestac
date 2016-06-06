#ifndef CPU_CPU_STRUCTS_H
#define CPU_CPU_STRUCTS_H

typedef struct {
    int Q;
    int QSleep;
    int pcb_size;
    void *serialized_pcb;
} t_kernel_data;

typedef struct {
    char** 	SEM_ID;
    char** 	SEM_INIT;
    char** 	IO_ID;
    char** 	IO_SLEEP;
    char** 	SHARED_VARS;
    int 	STACK_SIZE;
    int 	PAGE_SIZE;
    int 	PUERTO_UMC;
    char*	IP_UMC;
    int 	PUERTO_KERNEL;
    char*	KERNEL_IP;
} t_setup;


#endif //CPU_CPU_STRUCTS_H
