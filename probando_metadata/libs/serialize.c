
#include "serialize.h"

/*
 * Serializa una cantidad de bytes de un objeto dentro de un buffer.
 *
 * object : El puntero al objeto que se quiere serializar.
 * nBytes : La cantidad de bytes del objeto que se quieren serializar.
 * buffer : El buffer donde se almacenaran los bytes seleccionados.
 * buffer_size : El tamanio del buffer, que terminara siendo el total de bytes almacenados en buffer.
 */
void serialize_data(void *object, t_size nBytes, char **buffer, t_size *buffer_size) {
    *buffer = (char*) realloc(*buffer, *buffer_size + nBytes);
    memcpy((*buffer + *buffer_size), object, nBytes);
    *buffer_size = *buffer_size + nBytes;
}

/*
 * Deserializa una cantidad de bytes de una sucesion de bytes serializados dentro de un buffer.
 *
 * object : El puntero al objeto donde se almacenaran los bytes.
 * nBytes : La cantidad de bytes del objeto que se quieren deserializar.
 * serialized_data : Puntero al conjunto de bytes serializados.
 * serialized_data_size : La cantidad de bytes restantes de serialized_data
 */
void deserialize_data(void *object, t_size nBytes, char **serialized_data, t_size *serialized_data_size) {
    object = malloc(nBytes);
    memcpy(object, *serialized_data, nBytes);
    *serialized_data = *serialized_data + nBytes;
    *serialized_data_size = *serialized_data_size - nBytes;
}