/*
 * kernel.h
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#ifndef KERNEL_KERNEL_H_
#define KERNEL_KERNEL_H_

#define UMC_ADDR 10.0.0.11
#define UMC_PORT 6061
#define KERNEL_ADDR 10.0.0.10
#define KERNEL_PORT 6060

#define NEXT_LINE_SIZE 50

typedef struct PCB
{
  int id;
  int pc;
  int codePags;
  int codeIndex;
  int segIndex;
  int stackIndex;
} PCB;

#endif /* KERNEL_KERNEL_H_ */
