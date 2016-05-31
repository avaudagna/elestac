#ifndef SERIALIZATION_SERIALIZE_H
#define SERIALIZATION_SERIALIZE_H

#include <string.h>
#include <stdlib.h>

int serialize_data(void *object, size_t nBytes, void **buffer, size_t *lastIndex);
int deserialize_data(void *object, size_t nBytes, void *serialized_data, size_t *lastIndex);

#endif //SERIALIZATION_SERIALIZE_H
