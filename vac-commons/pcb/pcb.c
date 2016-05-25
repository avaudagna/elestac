
#include "pcb.h"

void serialize_enum_queue(enum_queue status, char **pString);

void serialize_pcb(t_pcb *pcb, char **buffer, t_size *buffer_size) {

    serialize_data(&pcb->pid, sizeof(pcb->pid), buffer, buffer_size);
    serialize_data(&pcb->program_counter, sizeof(pcb->program_counter), buffer, buffer_size);
    serialize_data(&pcb->program_counter, sizeof(pcb->stack_pointer), buffer, buffer_size);
    serialize_stack(&pcb->stack_index, buffer, buffer_size);
    serialize_data(&pcb->status, sizeof(pcb->status), buffer, buffer_size);
    serialize_data(&pcb->instrucciones_size, sizeof(pcb->instrucciones_size), buffer, buffer_size);
    serialize_data(&pcb->instrucciones_serializado, pcb->instrucciones_size, buffer, buffer_size);
}


