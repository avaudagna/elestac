#ifndef SERIALIZATION_PCBDESERIALIZATION_H
#define SERIALIZATION_PCBDESERIALIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <parser/metadata_program.h>

void deserialize_t_metadata_program(t_metadata_program **x, char *serialized_data);

#endif //SERIALIZATION_PCBDESERIALIZATION_H
