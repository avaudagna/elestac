
#include "serialize.h"

/*
 * Serializa una cantidad de bytes de un objeto dentro de un buffer.
 *
 * object : El puntero al objeto que se quiere serializar.
 * nBytes : La cantidad de bytes del objeto que se quieren serializar.
 * buffer : El buffer donde se almacenaran los bytes seleccionados.
 * buffer_size : El tamanio del buffer, que terminara siendo el total de bytes almacenados en buffer.
 */
int serialize_data(void *object, int nBytes, void **buffer, int *lastIndex) {
    void * auxiliar = NULL;
    auxiliar  = realloc(*buffer, nBytes+*lastIndex);
    if(auxiliar  == NULL) {
        return -1;
    }
    *buffer = auxiliar;
    if (memcpy((*buffer + *lastIndex), object, nBytes) == NULL) {
        return -2;
    }
    *lastIndex += nBytes;
    return 0;

}

/*
 * Deserializa una cantidad de bytes de una sucesion de bytes serializados dentro de un buffer.
 *
 * object : El puntero al objeto donde se almacenaran los bytes.
 * nBytes : La cantidad de bytes del objeto que se quieren deserializar.
 * serialized_data : Puntero al conjunto de bytes serializados.
 * serialized_data_size : La cantidad de bytes restantes de serialized_data
 */
int deserialize_data(void *object, int nBytes, void *serialized_data, int *lastIndex) {
    if(memcpy(object, serialized_data + *lastIndex, nBytes) == NULL) {
        return -2;
    }
    *lastIndex = *lastIndex + nBytes;
    return 0;
}
