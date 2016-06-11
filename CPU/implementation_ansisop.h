#ifndef IMPL_ANSISOP_H_
#define IMPL_ANSISOP_H_

#include <parser/parser.h>

#include <stdio.h>
#include "libs/pcb.h"
#include "cpu.h"
#include "cpu_structs.h"

#define SUCCESS 0
#define ERROR -1

#define ANSISOP_VAR_SIZE 4

#define PAGINA_INVALIDA_ID "2"
#define PAGINA_VALIDA_ID "1"
#define STACK_OVERFLOW_ID "3"
#define OPERACION_EXITOSA_ID "1"

#define ENTRADA_SALIDA_ID "3"

    //typedef t_var* t_posicion;

    typedef t_puntero t_posicion; // el foro dice que es  ((n° de pagina) * tamaño de pagina) + offset

    extern int umcSocketClient;
	extern int kernelSocketClient;
	extern t_pcb* actual_pcb;
    extern t_setup * setup;
    extern t_log* cpu_log;
    extern t_kernel_data* actual_kernel_data;

    //#1
    t_posicion definirVariable(t_nombre_variable variable);

    logical_addr * armar_direccion_logica(int pointer, int size);
    int add_stack_variable(int *stack_pointer, t_stack **stack, t_var *nueva_variable);
    t_posicion get_t_posicion(const t_var *nueva_variable);

    //#2
    t_posicion obtenerPosicionVariable(t_nombre_variable variable);

    //#3
    t_valor_variable dereferenciar(t_posicion direccion_variable);
    void obtain_Logical_Address(logical_addr* direccion, t_puntero posicion);

    //#4
    void asignar(t_posicion direccion_variable, t_valor_variable valor);

    //#5
    int imprimir(t_valor_variable valor);

    //#6
    int imprimirTexto(char* texto);

    //#7
    void irAlLabel(t_nombre_etiqueta etiqueta);

    //#8
    void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);

    //#9
    void retornar(t_valor_variable retorno);
    t_posicion  obtener_t_posicion(logical_addr *address);

    //#12
    void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);


#endif
