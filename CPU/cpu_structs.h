#ifndef CPU_CPU_STRUCTS_H
#define CPU_CPU_STRUCTS_H

typedef struct {
    int Q;
    int QSleep;
    int pcb_size;
    void *serialized_pcb;
} t_kernel_data;


#endif //CPU_CPU_STRUCTS_H
