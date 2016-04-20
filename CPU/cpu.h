#ifndef CPU_H
#define CPU_H

#define UMC_ADDR 10.0.0.11
#define UMC_PORT 6061
#define KERNEL_ADDR 10.0.0.10
#define KERNEL_PORT 6060

#define NEXT_LINE_SIZE 50

typedef struct PCB
{
  int id,
  int pc,
  int codePags,
  int codeIndex,
  int segIndex,
  int stackIndex
} PCB;

void incrementPC(PCB* incoming_pcb);
void getNextLine(int umcClient,int codeIndex, char* nextLine);
void getParsedInstructions(char* nextLine, char** parsedLines);
void executeParsedLines (char* parsedLines);
void getNextLine(int umcClient,int codeIndex, char* nextLine);
void getKernelClient(int* sock);
void getUmcClient(int* sock);

#endif
