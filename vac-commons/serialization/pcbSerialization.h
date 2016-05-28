#ifndef SERIALIZATION_PCBSERIALIZATION_H
#define SERIALIZATION_PCBSERIALIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <parser/metadata_program.h>
#include <commons/collections/queue.h>

typedef enum {NEW, READY, EXECUTING, BLOCKED, EXIT} enum_queue;

typedef struct {
    uint32_t pid;
    uint32_t program_counter;
    uint32_t stack_pointer;
    t_queue* stack_queue;
    enum_queue status;
    t_intructions instrucciones_serializado;
    t_size instrucciones_size;
    t_size			etiquetas_size;
    char*			etiquetas
} t_pcb;

void serialize_t_metadata_program(t_pcb *x, char **output;
void serialize_etiquetas(char *etiquetas, size_t etiquetas_size, char **buffer, int* output_index);
void serialize_t_intructions(t_intructions* instrucciones_serializado, t_size size, char** buffer, int* output_index)
void serlize_t_size(t_size instrucciones_size, char** buffer, int* output_index);

void serialize_int(int num, char** buffer, int* output_index);

#endif //SERIALIZATION_PCBSERIALIZATION_H
