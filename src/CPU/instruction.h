#include "CPU.h"

/* Accepts a 16-bit opcode and checks if it corresponds to a two-word instruction */
bool isTwoWordInstruction(u16 opcode);

/* Where the opcode rubber meets the memory road */
void avrInstruction(CPU *cpu);