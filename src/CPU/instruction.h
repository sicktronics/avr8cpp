#include "CPU.h"

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  Header file for the instruction module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

/* 
    @param opcode: 16-bit opcode and checks if it corresponds to a two-word instruction
    @returns True if the opcode is a two-word instruction
*/
bool isTwoWordInstruction(u16 opcode);

/* Where the opcode rubber meets the memory road */
void avrInstruction(CPU *cpu);

/* Delaying each instruction cycle for approprate amount of time*/
void delayNanoSec(int iterations);



