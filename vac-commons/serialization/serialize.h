#ifndef SERIALIZATION_SERIALIZE_H
#define SERIALIZATION_SERIALIZE_H

#include <parser/parser.h>

void serialize_data(void *object, t_size nBytes, char **buffer, t_size *buffer_size);
void deserialize_data(void *object, t_size nBytes, char **serialized_data, t_size *serialized_data_size);

#endif //SERIALIZATION_SERIALIZE_H
