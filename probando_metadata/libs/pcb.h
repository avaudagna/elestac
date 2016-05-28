#ifndef SERIALIZATION_PCB_H
#define SERIALIZATION_PCB_H

#include <elf.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include "serialize.h"
#include "stack.h"

typedef enum {NEW, READY, EXECUTING, BLOCKED, EXIT} enum_queue;
typedef struct {
    pid_t pid;
    uint32_t program_counter;
    uint32_t stack_pointer;
    t_stack  *stack_index;
    int status;
    t_size instrucciones_size;
    t_intructions* instrucciones_serializado;
    t_size etiquetas_size;
    char* etiquetas;
} t_pcb;

void serialize_pcb(t_pcb *pcb, void **buffer, size_t *buffer_size);
void deserialize_pcb(t_pcb **pcb, void *serialized_data, size_t *serialized_data_size);

void serialize_t_instructions(t_intructions *intructions, void **buffer, size_t *buffer_size);
void deserialize_instrucciones(t_intructions **intructions, t_size instrucciones_size, void **serialized_data, size_t *serialized_data_size);

void serialize_instrucciones(t_intructions *instrucciones, size_t instrucciones_size, void **pVoid, size_t *pInt);
//deserialize

//serialize_etiquetas
//deserialize_etiquetas


#endif //SERIALIZATION_PCB_H
