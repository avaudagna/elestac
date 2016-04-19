#ifndef CPU_H
#define CPU_H

typedef struct PCB
{
  int id,
  int pc,
  int codePags,
  int codeIndex,
  int segIndex,
  int stackIndex
} PCB;

#endif
