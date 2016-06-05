/*
 * Copyright (C) 2012 Sistemas Operativos - UTN FRBA. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "implementation_ansisop.h"
#include "libs/socketCommons.h"


static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;

t_puntero definirVariable(t_nombre_variable variable) {

	int page_number, offset, tamanio;

	//Leo el indice del stack,

	//Pido a UMC espacio en memoria(Codigo: 3) con 4bytes
	asprintf(&buffer, "3%04d%04d%04d0000", page_number, offset, tamanio);
	if( send(umcSocketClient, buffer, 12, 0) < 0) {
		puts("Send addr instruction_addr");
		return 1;
	}

	recv_bytes_buffer = calloc(1, (size_t) element->tamanio);
	if( recv(umcSocketClient , recv_bytes_buffer , (size_t ) element->tamanio , 0) < 0) {
		log_error(cpu_log, "UMC bytes recv failed");
		return -1;
	}

	t_var* var = (t_var*) calloc(1,sizeof(t_var));

	var->var_id = variable;
	var->page_number = page_number;
	var->offset = offset;
	var->tamanio = tamanio;

	return POSICION_MEMORIA;
}

t_puntero obtenerPosicionVariable(t_nombre_variable variable) {
	printf("Obtener posicion de %c\n", variable);
	return POSICION_MEMORIA;
}

t_valor_variable dereferenciar(t_puntero puntero) {
	printf("Dereferenciar %d y su valor es: %d\n", puntero, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

void asignar(t_puntero puntero, t_valor_variable variable) {
	printf("Asignando en %d el valor %d\n", puntero, variable);
}

void imprimir(t_valor_variable valor) {
	printf("Imprimir %d\n", valor);
}

void imprimirTexto(char* texto) {
	printf("ImprimirTexto: %s", texto);
}
