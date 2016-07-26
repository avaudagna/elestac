#ifndef SERIALIZATION_PCB_H
#define SERIALIZATION_PCB_H

#include <elf.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include "serialize.h"
#include "stack.h"

typedef enum {NEW, READY, EXECUTING, BLOCKED, EXIT, BROKEN, WAITING, ABORTED} enum_queue;
typedef struct {
    int pid;
    int program_counter;
    int stack_pointer;
    t_stack  *stack_index;
    int status;
    int instrucciones_size;
    t_intructions* instrucciones_serializado;
    int etiquetas_size;
    char* etiquetas;
} t_pcb;

void serialize_pcb(t_pcb *pcb, void **buffer, int *buffer_size);
void deserialize_pcb(t_pcb **pcb, void *serialized_data, int *serialized_data_index);

void deserialize_instrucciones(t_intructions **intructions, int instrucciones_size, void **serialized_data, int *serialized_data_size);
void serialize_instrucciones(t_intructions *instrucciones, int instrucciones_size, void **pVoid, int *pInt);

void serialize_t_instructions(t_intructions *intructions, void **buffer, int *buffer_size);
void deserialize_etiquetas(char **etiquetas, int etiquetas_size, void *serialized_data, int *serialized_data_index);


#endif //SERIALIZATION_PCB_H
