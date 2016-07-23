#ifndef SERIALIZATION_SERIALIZE_H
#define SERIALIZATION_SERIALIZE_H

#include <string.h>
#include <stdlib.h>

int serialize_data(void *object, int nBytes, void **buffer, int *lastIndex);
int deserialize_data(void *object, int nBytes, void *serialized_data, int *lastIndex);

#endif //SERIALIZATION_SERIALIZE_H
