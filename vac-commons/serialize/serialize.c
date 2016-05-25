
#include "serialize.h"

void serialize_data(void *object, size_t nBytes, char **buffer, t_size *buffer_size) {
    *buffer = (char*) realloc(*buffer, *buffer_size + nBytes);
    memcpy((*buffer + *buffer_size), object, nBytes);
    *buffer_size = *buffer_size + nBytes;
}