//
// Created by alan on 25/05/16.
//

#ifndef SERIALIZATION_STACK_H
#define SERIALIZATION_STACK_H

#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <parser/parser.h>
#include "serialize.h"

typedef t_queue t_stack;

typedef struct {
    int pos;
    int cant_args;
    char *args; //12 bytes por arg
    int cant_vars;
    char *vars; //13 bytes por var
    int ret_pos;
    int cant_ret_vars;
    char *ret_vars;// 12 bytes por ret_var
} t_stack_entry;

void serialize_stack (t_stack *stack, char **buffer, t_size *buffer_size);
void append_stack_entry(char **list_buffer, char *item_buffer, t_size item_size,
                        t_size *list_buffer_size);
void serialize_stack_entry(t_stack_entry *entry, char **buffer, t_size *buffer_size);


#endif //SERIALIZATION_STACK_H
