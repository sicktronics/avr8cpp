#include "CPU.h"
#pragma once

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  Header for the interrupt module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

/*
    This function emulates how the CPU would handle an interrupt by saving the current program counter (PC) to the stack, updating the stack pointer, and jumping to a specified address (interrupt vector).
    @param cpu: Pointer to a CPU
    @param address: Address of the interrupt
    @returns No return value
*/
void avrInterrupt(CPU *cpu, u16 address);