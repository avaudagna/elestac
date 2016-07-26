#ifndef IMPL_ANSISOP_H_
#define IMPL_ANSISOP_H_

#include <parser/parser.h>

#include <stdio.h>
#include "libs/pcb.h"
#include "cpu.h"
#include "cpu_structs.h"
#include <parser/metadata_program.h>

#define SUCCESS 1
#define ERROR -1

#define ANSISOP_VAR_SIZE 4

#define PAGINA_INVALIDA_ID '2'
#define PAGINA_VALIDA_ID '1'
#define STACK_OVERFLOW_ID '3'
#define OPERACION_EXITOSA_ID 1

#define SHARED_VAR_ID '5'
#define GET_VAR '0'
#define SET_VAR '1'

#define ENTRADA_SALIDA_ID '3'

#define IMPRIMIR_ID '6'
#define IMPRIMIR_TEXTO_ID '7'

#define SEMAPHORE_ID '4'
#define WAIT_ID '0'
#define SIGNAL_ID '1'

#define STACK_OVERFLOW -2


//UMC Interface
#define HANDSHAKE_CPU 1
#define CAMBIO_PROCESO_ACTIVO '2'
#define PEDIDO_BYTES '3'
#define ALMACENAMIENTO_BYTES '4'
#define FIN_COMUNICACION_CPU '0'

    //typedef t_var* t_posicion;

    typedef t_puntero t_posicion; // el foro dice que es  ((n° de pagina) * tamaño de pagina) + offset

typedef struct {
    int data_length;
    void * data;
} t_nodo_send;

    extern int umcSocketClient;
	extern int kernelSocketClient;
	extern t_pcb* actual_pcb;
    extern t_setup * setup;
    extern t_log* cpu_log;
    extern t_kernel_data* actual_kernel_data;

    //#1
    t_posicion definirVariable(t_nombre_variable variable);

    logical_addr * armar_direccion_logica_variable(int pointer, int size);
    void armar_pedidos_escritura(t_list ** pedidos, t_list *direcciones, int valor);
    t_posicion get_t_posicion(const t_var *nueva_variable);

    //#2
    t_posicion obtenerPosicionVariable(t_nombre_variable variable);

    //#3
    t_valor_variable dereferenciar(t_posicion direccion_variable);
    void obtain_Logical_Address(logical_addr* direccion, t_puntero posicion);
    void construir_operaciones_lectura(t_list **pedidos, t_posicion posicion_variable);

    //#4
    void asignar(t_posicion direccion_variable, t_valor_variable valor);
    void obtener_lista_operaciones_escritura(t_list ** pedidos, t_posicion posicion_variable, int offset, int valor);

    //#5
    t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);

    //#6
    t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);

    //#7
    void irAlLabel(t_nombre_etiqueta etiqueta);

    //#8
    void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
    void llamarSinRetorno (t_nombre_etiqueta etiqueta);

    //#9
    void retornar(t_valor_variable retorno);
    t_posicion obtener_t_posicion(logical_addr *address);

    //#10
    void imprimir(t_valor_variable valor);

    //#11
    void imprimirTexto(char* texto);

    //#12
    void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);

    //#13
    void la_wait(t_nombre_semaforo identificador_semaforo);

    //#14
    void la_signal(t_nombre_semaforo identificador_semaforo);

    void stack_overflow_exit();
    char recv_umc_response_status();

#endif
