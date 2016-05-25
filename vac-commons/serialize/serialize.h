#ifndef SERIALIZATION_SERIALIZE_H
#define SERIALIZATION_SERIALIZE_H

#include <parser/parser.h>

void serialize_data(void *object, size_t nBytes, char **buffer, t_size *buffer_size);

#endif //SERIALIZATION_SERIALIZE_H
