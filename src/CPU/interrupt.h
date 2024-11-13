#include "CPU.h"

#pragma once 

/*
This function emulates how the CPU would handle an interrupt by saving the current program counter (PC) to the stack, updating the stack pointer, and jumping to a specified address (interrupt vector).
*/
void avrInterrupt(CPU *cpu, u16 address);