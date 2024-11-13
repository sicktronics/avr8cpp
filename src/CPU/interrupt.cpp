#include "interrupt.h"

void avrInterrupt(CPU *cpu, u16 address) {

    // get the contents of the stack pointer
    u16 SP = cpu->getSP();

    // Save the address of the next instruction (what PC currently points to)
    cpu->data[SP] = cpu->PC & 0xff; // LSB
    cpu->data[SP - 1] = (cpu->PC >> 8) & 0xff; // MSB stored at top of the stack - 1

    // If we can address 22bits, store the MSB at top of stack -2
    if(cpu->pc22Bits) {
        cpu->data[SP - 2] = (cpu->PC >> 16) & 0xff;
    }

    // After saving PC on the stack, the stack pointer is decremented by 2 or 3 bytes (depending on the program 
    // counter width) to point to the new top of the stack.
    if(cpu->pc22Bits) {
        cpu->setSP(SP - 3);
    }
    else {
        cpu->setSP(SP - 2);
    }

    // This line clears the I flag (bit 7 of SREG) by performing a bitwise AND with 0x7f (binary 0b01111111), 
    // disabling further interrupts until the flag is set again.
    cpu->data[95] &= 0b01111111;

    // Simulate the number of cycles it would take the CPU to do this
    cpu->cycles += 2;

    // Set PC to addr, which is the interrupt vector address where the interrupt handling routine is located.
    // This change in PC redirects the CPU to start executing the interrupt service routine (ISR).
    cpu->PC = address;
}