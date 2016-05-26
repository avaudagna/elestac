
#include "stack.h"

/*
 * Serializa las entradas del stack en una char*.
 *
 * stack  : El stack que se quiere serializar.
 * buffer : El buffer donde se quiere almacenar el stack serializado.
 * buffer_size : Variable donde se almacenara el tamanio final que terminara teniendo el buffer.
 */
void serialize_stack (t_stack *stack, char **buffer, t_size *buffer_size) {

    //1) Agarrro el t_list elements (t_link_element {t_link_element *head, int elements_count})
    //2) voy recorriendo agarrando head y sumandole 1
    //(t_link_element {void *data, t_link_element *next}) , next sera NULL para el ultimo elemento
    //3) a cada link element tengo que serializarle el data, que va a ser un t_stack_entry
    //(t_stack_entry {int pos, int cant_args, char *args, int cant_vars, char *vars,
    //                int ret_pos, int cant_ret_vars, char *ret_vars})

    t_link_element *link_actual = NULL;
    t_stack_entry *entrada_actual = NULL;
    char *stack_entry_item_buffer = NULL, *stack_entry_list_buffer = *buffer, *next = NULL;
    u_int32_t cantidad_elementos_stack = 0;
    t_stack_entry *data = NULL;
    u_int32_t indice = 0;
    t_size stack_entry_item_size = 0, stack_entry_list_buffer_size = *buffer_size;
    //Stack / Queue
    t_list *elementos = stack-> elements; //Tomo la lista de elementos de la queue
    t_link_element *element = elementos->head; //Tomo el primer elemento de la lista

    //Lista de la Queue
    link_actual = elementos->head; //Tomo la data del primer link de la lista
    cantidad_elementos_stack = elementos->elements_count; //Cantidad de links totales en el stack

    if(link_actual == NULL && cantidad_elementos_stack > 0) {
        return;
    }
    //El primer elemento que se agrega es la cantidad de elementos totales que tendra el stack
    serialize_data(&cantidad_elementos_stack,sizeof(cantidad_elementos_stack),
                   &stack_entry_list_buffer, &stack_entry_list_buffer_size);

    for(indice = 0; indice < cantidad_elementos_stack; indice++) {
        entrada_actual = (t_stack_entry*) link_actual->data; //Tomo la data del primer link de la lista
        serialize_stack_entry(entrada_actual, &stack_entry_item_buffer, &stack_entry_item_size);
        link_actual = link_actual->next; //En la ultima iteracion, entrada_actual terminara siendo igualada a NULL
        append_stack_entry(&stack_entry_list_buffer, stack_entry_item_buffer, stack_entry_item_size,
                           &stack_entry_list_buffer_size);
    }
    //Ya se leyeron todos los datos (t_stack_entry) del stack y se los serializaron el buffer
    //Tendria que haber cantidad_elementos_stack elementos en stack_entry_list_buffer
    //y stack_entry_list_buffer_size tendria que tener el size total del stack_entry_list_buffer
}

/*
 * Agrega elementos a al buffer de stack, tiene el formato: itemSize|item.
 *
 * list_buffer : Donde se almacena toda la lista de entradas.
 * item_buffer : Elemento a agregar a la lista.
 * item_size   : Tamanio del item a agregar.
 * list_buffer_size : Tamanio del buffer hasta el momento.
 */
void append_stack_entry(char **list_buffer, char *item_buffer, t_size item_size,
                        t_size *list_buffer_size) {
    //Append sizeof del item
    list_buffer = realloc(*list_buffer, *list_buffer_size + sizeof(t_size));
    *list_buffer_size = *list_buffer_size + sizeof(t_size);
    memcpy(*list_buffer + *list_buffer_size, (void*) sizeof(t_size), item_size );
    //Append stack_entry
    list_buffer = realloc(*list_buffer, *list_buffer_size + item_size);
    *list_buffer_size = *list_buffer_size + item_size;
    memcpy( *list_buffer + *list_buffer_size, item_buffer, item_size);
}

/*
 * Serializa una entrada del stack a un buffer, almacenando su tamanio.
 *
 * serialized_data format >> pos|cant_args|args|cant_vars|vars|cant_ret_vars|ret_vars.
 *
 * entry  : La entrada que se quiere serializar.
 * buffer : El buffer donde se almacenara la entrada serializada.
 * buffer_size : Variable que terminara con el valor del tamanio final del buffer.
 */
void serialize_stack_entry(t_stack_entry *entry, char **buffer, t_size *buffer_size) {

    serialize_data(&entry->pos, sizeof(entry->pos), buffer, buffer_size);
    serialize_data(&entry->cant_args, sizeof(entry->cant_args), buffer, buffer_size);
    serialize_data(&entry->args, (t_size) entry->cant_args, buffer, buffer_size);
    serialize_data(&entry->cant_vars, sizeof(entry->cant_vars), buffer, buffer_size);
    serialize_data(&entry->vars, (t_size) entry->cant_vars, buffer, buffer_size);
    serialize_data(&entry->cant_ret_vars, sizeof(entry->cant_vars), buffer, buffer_size);
    serialize_data(&entry->ret_vars, (t_size) entry->cant_vars, buffer, buffer_size);
}

/*
 * Deserializa un t_stack de un conjunto de bytes.
 *
 * serialized_data format >> cantidadLinks|[entrySize(0)|entry(0)...entrySize(cantidadLinks-1)|entry(cantidadLinks-1)]
 *
 * **stack    : Donde el stack se almacenara.
 * serialized_data : Conjunto de bytes serializados.
 * serialized_data_size : Tamanio total del conjunto de bytes.
 */
void deserialize_stack(t_stack *stack, char **serialized_data, t_size *serialized_data_size) {
    u_int32_t cantidad_links = 0, indice = 0;
    t_stack_entry *stack_entry = NULL;

    stack = queue_create(); //Creo el stack

    deserialize_data(&cantidad_links, sizeof(cantidad_links), serialized_data, serialized_data_size);
    for(indice = 0; indice < cantidad_links; indice++) {
        deserialize_stack_entry(&stack_entry, serialized_data, serialized_data_size);
        queue_push(stack, stack_entry); //Agrego elementos al stack
    }
}

/*
 * Deserializa un t_stack_entry de un conjunto de bytes.
 *
 * serialized_data format >> pos|cant_args|args|cant_vars|vars|cant_ret_vars|ret_vars
 *
 * **entry    : Donde el stack_entry se almacenara.
 * serialized_data : Conjunto de bytes serializados.
 * serialized_data_size : Tamanio total del conjunto de bytes.
 */
    void deserialize_stack_entry(t_stack_entry **entry, char **serialized_data, t_size *serialized_data_size) {

    deserialize_data(&(*entry)->pos, sizeof((*entry)->pos), serialized_data, serialized_data_size);
    deserialize_data(&(*entry)->cant_args, sizeof((*entry)->cant_args), serialized_data, serialized_data_size);
    deserialize_data((*entry)->args, (t_size) (*entry)->cant_args, serialized_data, serialized_data_size);
    deserialize_data(&(*entry)->cant_vars, sizeof((*entry)->cant_vars), serialized_data, serialized_data_size);
    deserialize_data((*entry)->vars, (t_size) (*entry)->cant_vars, serialized_data, serialized_data_size);
    deserialize_data(&(*entry)->cant_ret_vars, sizeof((*entry)->cant_vars), serialized_data, serialized_data_size);
    deserialize_data((*entry)->ret_vars, (t_size) (*entry)->cant_vars, serialized_data, serialized_data_size);
}