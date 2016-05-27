
#include "pcb.h"

/*
 * Serializa un pcb a una cadena de bytes.
 *
 * pcb : El puntero al pcb que se quiere serializar.
 * buffer : El buffer donde se almacenaran los bytes del pcb.
 * buffer_size : El tamanio del buffer, que terminara siendo el total de bytes que componen al pcb.
 */
void serialize_pcb(t_pcb *pcb, char **buffer, t_size *buffer_size) {

    serialize_data(&pcb->pid, sizeof(pcb->pid), buffer, buffer_size);
    serialize_data(&pcb->program_counter, sizeof(pcb->program_counter), buffer, buffer_size);
    serialize_data(&pcb->program_counter, sizeof(pcb->stack_pointer), buffer, buffer_size);
    serialize_stack(pcb->stack_index, buffer, buffer_size);
    serialize_data(&pcb->status, sizeof(pcb->status), buffer, buffer_size);
    serialize_data(&pcb->instrucciones_size, sizeof(pcb->instrucciones_size), buffer, buffer_size);
    serialize_data(&pcb->instrucciones_serializado, pcb->instrucciones_size, buffer, buffer_size);
}

/*
 * Deserializa un pcb a una cadena de bytes.
 *
 * pcb : El puntero al pcb que se habra deserializado.
 * serialized_data : El buffer donde estaran almacenados los bytes del pcb.
 * serialized_data_size : La cantidad de bytes almacenados en serialized_data.
 */
void deserialize_pcb(t_pcb **pcb, char **serialized_data, t_size *serialized_data_size) {

    deserialize_data(&(*pcb)->pid, sizeof((*pcb)->pid), serialized_data, serialized_data_size);
    deserialize_data(&(*pcb)->program_counter, sizeof((*pcb)->program_counter), serialized_data, serialized_data_size);
    deserialize_data(&(*pcb)->program_counter, sizeof((*pcb)->stack_pointer), serialized_data, serialized_data_size);
    deserialize_stack(&(*pcb)->stack_index, serialized_data, serialized_data_size);
    deserialize_data(&(*pcb)->status, sizeof((*pcb)->status), serialized_data, serialized_data_size);
    deserialize_data(&(*pcb)->instrucciones_size, sizeof((*pcb)->instrucciones_size), serialized_data, serialized_data_size);
    deserialize_data(&(*pcb)->instrucciones_serializado, (*pcb)->instrucciones_size, serialized_data, serialized_data_size);
}



void serialize_instrucciones(t_intructions *intructions, char **buffer, t_size *buffer_size) {
    serialize_data(&intructions->start, sizeof(intructions->start), buffer, buffer_size);
    serialize_data(&intructions->offset, sizeof(intructions->offset), buffer, buffer_size);
}

void deserialize_instrucciones(t_intructions **intructions, char **serialized_data, t_size *serialized_data_size) {
    deserialize_data(&(*intructions)->start, sizeof((*intructions)->start), serialized_data, serialized_data_size);
    deserialize_data(&(*intructions)->offset, sizeof((*intructions)->offset), serialized_data, serialized_data_size);
}
