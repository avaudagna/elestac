#ifndef IMPL_ANSISOP_H_
#define IMPL_ANSISOP_H_

#include <parser/parser.h>

#include <stdio.h>
#include "libs/pcb.h"
#include "cpu.h"
#include "cpu_structs.h"


#define ANSISOP_VAR_SIZE 4


    typedef t_var* t_posicion;

    extern int umcSocketClient;
	extern int kernelSocketClient;
	extern t_pcb* actual_pcb;
    extern t_setup * setup;
    extern t_log* cpu_log;
    extern t_kernel_data* actual_kernel_data;

    //#1
    t_puntero definirVariable(t_nombre_variable variable);

    logical_addr * armar_direccion_logica(int pointer, int size);
    int add_stack_variable(int *stack_pointer, t_stack **stack, t_var *nueva_variable);

    //#2
    t_puntero obtenerPosicionVariable(t_nombre_variable variable);

    //#3
    t_valor_variable dereferenciar(t_puntero puntero);

    //#4
    void asignar(t_puntero puntero, t_valor_variable variable);

    //#5
    void imprimir(t_valor_variable valor);

    //#6
    void imprimirTexto(char* texto);

#endif
