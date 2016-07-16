
#ifndef PCB_TESTS_H_
#define PCB_TESTS_H_

#include <stdio.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <commons/collections/queue.h>
//TODO: DESCOMENTAR ESTA LINEA EN CASO DE QUERER DEBUGGEAR Y VER QUE IMPRIME PARA ENCONTRAR EL ERROR
#define NDEBUG
#include <assert.h>
#include "pcb.h"

t_metadata_program *getMetadataExample();
t_stack *getStackExample();

int printStackValuesVsBuffer(t_stack *stack, void *buffer);
void printStackEntryVsBuffer(t_stack_entry *entry, void *buffer, int *buffer_index);

int printInstructions(t_intructions *serializado, int size, void *pVoid);
void printEtiquetas(char *etiquetas, char *buffer);
void print_instrucciones_size ();

void printStackValuesVsStruct(t_stack *index, t_stack *stack_index);

void printStackEntryVsEntry(t_stack_entry *orig, t_stack_entry *aNew);

void testOldPCBvsNewPCB(t_pcb *newPCB, t_pcb *incomingPCB);

void testSerializedPCB(t_pcb *newPCB, void *pcb_buffer);

void printSerializedPcb(void *pcb_buffer);
int printBufferStackValues( void *buffer) ;
void printBufferStackEntry(void *buffer, int *buffer_index) ;
int printBufferInstructions(int cant_instrucciones, void *buffer) ;
int printBufferEtiquetas(char *buffer, int cant_etiquetas) ;
t_pcb * getPcbExample();

#endif
