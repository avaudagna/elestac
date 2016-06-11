#ifndef SERIALIZATION_STACK_H
#define SERIALIZATION_STACK_H

#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <parser/parser.h>
#include "serialize.h"

#define ERROR -1
#define SUCCESS 1

typedef struct {
    int page_number;
    int offset;
    int tamanio;
} logical_addr;

typedef struct {
    char var_id;
    int page_number;
    int offset;
    int tamanio;
} t_var;

typedef t_queue t_stack;
typedef logical_addr t_arg;
typedef logical_addr t_ret_var;

typedef struct {
    int pos;
    int cant_args;
    t_arg *args; //12 bytes por arg
    int cant_vars;
    t_var *vars; //13 bytes por var
    int cant_ret_vars;
    t_ret_var *ret_vars; // 12 bytes por ret_var
    int ret_pos;
} t_stack_entry;


void serialize_stack (t_stack *stack, void **buffer, int *buffer_size);
void serialize_stack_entry(t_stack_entry *entry, void **buffer, int *buffer_size);
t_stack_entry *create_new_stack_entry();
void deserialize_stack(t_stack **stack, void **serialized_data, int *serialized_data_size);
void deserialize_stack_entry(t_stack_entry **entry, void **serialized_data, int *serialized_data_size);

int add_ret_var(t_stack_entry ** stack_entry, t_ret_var *var);
int add_var(t_stack_entry ** stack_entry, t_var *var);
int add_arg(t_stack_entry ** stack_entry, t_arg *arg);

t_stack_entry * stack_entry_create(void);

#endif //SERIALIZATION_STACK_H
