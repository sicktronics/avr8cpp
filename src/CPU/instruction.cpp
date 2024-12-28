#include "instruction.h"


bool isTwoWordInstruction(u16 opcode) {
    return (
        // LDS instruction
        (opcode & 0xFE0F) == 0x9000 ||
        // STS instruction
        (opcode & 0xFE0F) == 0x9200 ||
        // CALL instruction
        (opcode & 0xFE0E) == 0x940E ||
        // JMP instruction
        (opcode & 0xFE0E) == 0x940C
    );
}

void avrInstruction(CPU *cpu){

    // Extract the opcode at the current pc
    /*
    const opcode = cpu.progMem[cpu.pc];
    */
    const u16 opcode = cpu->programMemory[cpu->PC];

    // Check for ADC opcode: 0001 11rd dddd rrrr
    if ((opcode & 0xFC00) == 0x1C00) {
        // Extract destination (d) and source (r) registers
        u8 dReg = (opcode & 0x1F0) >> 4;
        u8 rReg = ((opcode & 0xF) | ((opcode & 0x200) >> 5));
        // Retrieve values from the data memory
        u8 d = cpu->data[dReg];
        u8 r = cpu->data[rReg];
        // Perform addition with carry
        u16 sum = d + r + (cpu->data[95] & 1); // Add carry flag (bit 0 of SREG)
        u8 result = sum & 0xFF;               // Result is lower 8 bits of sum
        // Update destination register with the result
        cpu->data[dReg] = result;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags (bits 6 and 7)
        sreg |= (result == 0) ? 2 : 0;  // Z (Zero) flag
        sreg |= (result & 0x80) ? 4 : 0; // N (Negative) flag
        sreg |= ((result ^ r) & (d ^ result) & 0x80) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (sum & 0x100) ? 1 : 0;  // C (Carry) flag
        sreg |= (1 & ((d & r) | (r & ~result) | (~result & d)) ) ? 0x20 : 0; // H (Half Carry) flag
        // Store updated SREG back into the CPU data memory
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFC00) == 0x0C00) {
    // ADD opcode: 0000 11rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 dReg = (opcode & 0x1F0) >> 4;
        u8 rReg = ((opcode & 0xF) | ((opcode & 0x200) >> 5));

        // Get the values from the registers
        u8 d = cpu->data[dReg];
        u8 r = cpu->data[rReg];

        // Perform the addition and mask to 8 bits
        u8 R = (d + r) & 0xFF;

        // Write the result back to the destination register
        cpu->data[dReg] = R;

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags (bits 6 and 7)
        sreg |= R ? 0 : 2;              // Z (Zero) flag
        sreg |= (128 & R) ? 4 : 0;      // N (Negative) flag
        sreg |= ((R ^ r) & (R ^ d) & 128) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= ((d + r) & 0x100) ? 1 : 0;  // C (Carry) flag
        sreg |= (1 & ((d & r) | (r & ~R) | (~R & d))) ? 0x20 : 0; // H (Half Carry) flag

        // Write the updated SREG back to data[95]
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFF00) == 0x9600) {
    // ADIW opcode: 1001 0110 KKdd KKKK

        // Calculate the address for the 16-bit register pair
        u16 addr = 2 * ((opcode & 0x30) >> 4) + 24;
        // Read the 16-bit value from the specified address
        u16 value = cpu->getUint16LittleEndian(addr);
        // Calculate the immediate value (K) from the opcode
        u16 K = (opcode & 0xF) | ((opcode & 0xC0) >> 2);
        // Perform the addition and limit to 16 bits
        u16 R = (value + K) & 0xFFFF;
        // Write the result back to the 16-bit register pair
        cpu->setUint16LittleEndian(addr, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE0; // Preserve I, T, and H flags (bits 5-7)
        sreg |= R ? 0 : 2;              // Z (Zero) flag
        sreg |= (R & 0x8000) ? 4 : 0;   // N (Negative) flag
        sreg |= (~value & R & 0x8000) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (~R & value & 0x8000) ? 1 : 0; // C (Carry) flag
        // Store the updated SREG back into memory
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFC00) == 0x2000) {
    // AND opcode: 0010 00rd dddd rrrr

        // Perform bitwise AND between the destination and source registers
        u8 dReg = (opcode & 0x1F0) >> 4; // Destination register index
        u8 rReg = (opcode & 0xF) | ((opcode & 0x200) >> 5); // Source register index
        u8 R = cpu->data[dReg] & cpu->data[rReg]; // Perform AND operation

        // Store the result back into the destination register
        cpu->data[dReg] = R;

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags (bits 5-7 and bit 0)
        sreg |= R ? 0 : 2;              // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag

        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xF000) == 0x7000) {
    // ANDI opcode: 0111 KKKK dddd KKKK

        // Perform bitwise AND between the register and the immediate value
        u8 dReg = ((opcode & 0xF0) >> 4) + 16; // Destination register index (R16 to R31)
        u8 K = (opcode & 0xF) | ((opcode & 0xF00) >> 4); // Immediate value (K)
        u8 R = cpu->data[dReg] & K; // Perform AND operation
        // Store the result back into the destination register
        cpu->data[dReg] = R;

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags (bits 5-7 and bit 0)
        sreg |= R ? 0 : 2;              // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag

        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFE0F) == 0x9405) {
    // ASR opcode: 1001 010d dddd 0101
        // Extract the destination register
        u8 dReg = (opcode & 0x1F0) >> 4;
        // Retrieve the value from the register
        u8 value = cpu->data[dReg];
        // Perform the arithmetic shift right operation
        u8 R = (value >> 1) | (value & 0x80); // Shift right and preserve MSB (sign extension)
        // Store the result back into the destination register
        cpu->data[dReg] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE0; // Preserve I, T, and H flags (bits 5-7)
        sreg |= R ? 0 : 2;              // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (value & 0x01);         // C (Carry) flag
        sreg |= (((sreg >> 2) & 1) ^ (sreg & 1)) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFF8F) == 0x9488) {
    // BCLR opcode: 1001 0100 1sss 1000

        // Clear the specified bit in SREG (data[95])
        cpu->data[95] &= ~(1 << ((opcode & 0x70) >> 4));
    }
    else if ((opcode & 0xFE08) == 0xF800) {
    // BLD opcode: 1111 100d dddd 0bbb

        // Extract bit position (b) and destination register (d)
        u8 b = opcode & 0x7;               // Bits [2:0]
        u8 d = (opcode & 0x1F0) >> 4;      // Bits [8:4]

        // Perform the bit load operation
        cpu->data[d] = (~(1 << b) & cpu->data[d]) | (((cpu->data[95] >> 6) & 1) << b);
    }
    else if ((opcode & 0xFC00) == 0xF400) {
    // BRBC opcode: 1111 01kk kkkk ksss

        // Extract bit number (sss) and offset (kkk)
        u8 s = opcode & 0x7;              // Bits [2:0] (bit to check in SREG)
        i8 k = ((opcode & 0x1F8) >> 3) - ((opcode & 0x200) ? 0x40 : 0); // Branch offset
        // Check if the specified bit in SREG is cleared
        if (!(cpu->data[95] & (1 << s))) {
            // Update the program counter with the branch offset
            cpu->PC += k;
            // Increment the cycle count
            cpu->cycles++;
        }
    }
    else if ((opcode & 0xFC00) == 0xF000) {
    // BRBS opcode: 1111 00kk kkkk ksss

        // Extract bit number (sss) and offset (kkk)
        u8 s = opcode & 0x7;              // Bits [2:0] (bit to check in SREG)
        i8 k = ((opcode & 0x1F8) >> 3) - ((opcode & 0x200) ? 0x40 : 0); // Branch offset
        // Check if the specified bit in SREG is set
        if (cpu->data[95] & (1 << s)) {
            // Update the program counter with the branch offset
            cpu->PC += k;
            // Increment the cycle count
            cpu->cycles++;
        }
    }
    else if ((opcode & 0xFF8F) == 0x9408) {
    // BSET opcode: 1001 0100 0sss 1000
        // Set the specified bit in SREG (data[95])
        cpu->data[95] |= (1 << ((opcode & 0x70) >> 4));
    }
    else if ((opcode & 0xFE08) == 0xFA00) {
    // BST opcode: 1111 101d dddd 0bbb

        // Extract the destination register (d) and the bit position (b)
        u8 d = cpu->data[(opcode & 0x1F0) >> 4]; // Destination register
        u8 b = opcode & 0x7;                     // Bit position
        // Store the specified bit from the register into the T flag in SREG
        cpu->data[95] = (cpu->data[95] & 0xBF) | (((d >> b) & 1) ? 0x40 : 0);
    }
    else if ((opcode & 0xFE0E) == 0x940E) {
    // CALL opcode: 1001 010k kkkk 111k kkkk kkkk kkkk kkkk

        // Calculate the absolute address to call (k)
        u32 k = cpu->programMemory[cpu->PC + 1] | 
                ((opcode & 0x1) << 16) | 
                ((opcode & 0x1F0) << 13);

        // Address of the return point (next instruction after CALL)
        u32 ret = cpu->PC + 2;
        // Get the current stack pointer value
        u16 sp = cpu->getSP();
        // Check if program counter is 22-bits
        bool pc22Bits = cpu->pc22Bits;
        // Push return address onto the stack
        cpu->data[sp] = ret & 0xFF;             // Lower byte
        cpu->data[sp - 1] = (ret >> 8) & 0xFF;  // Upper byte
        if (pc22Bits) {
            cpu->data[sp - 2] = (ret >> 16) & 0xFF; // High byte for 22-bit PC
        }
        // Update the stack pointer
        cpu->setSP(sp - (pc22Bits ? 3 : 2));
        // Set the program counter to the target address
        cpu->PC = k - 1; // Decrement to account for increment after instruction execution
        // Update the cycle count
        cpu->cycles += pc22Bits ? 4 : 3;
    }
    else if ((opcode & 0xFF00) == 0x9800) {
    // CBI opcode: 1001 1000 AAAA Abbb

        // Extract I/O register address (A) and bit position (b)
        u8 A = opcode & 0xF8;          // Bits [10:3] (aligned to 8-bit I/O space)
        u8 b = opcode & 0x07;          // Bits [2:0] (bit position)
        // Calculate the I/O register address
        u16 ioAddress = (A >> 3) + 32;
        // Read the value from the I/O register
        u8 R = cpu->readData(ioAddress);
        // Create a mask for the bit to clear
        u8 mask = 1 << b;
        // Write the updated value back to the I/O register, clearing the specified bit
        cpu->writeData(ioAddress, R & ~mask, mask);
    }
    else if ((opcode & 0xFE0F) == 0x9400) {
    // COM opcode: 1001 010d dddd 0000

        // Extract the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        // Perform the one's complement operation
        u8 R = 255 - cpu->data[d];
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = (cpu->data[95] & 0xE1) | 1; // Preserve I, T, H and set C (Carry) flag
        sreg |= R ? 0 : 2;                    // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;           // N (Negative) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFC00) == 0x1400) {
    // CP opcode: 0001 01rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4;                       // Destination register index
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5);    // Source register index
        // Read the values from the registers
        u8 val1 = cpu->data[d];
        u8 val2 = cpu->data[r];
        // Perform the subtraction
        i16 R = val1 - val2; // Use i16 to handle potential negative results
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags (bits 6 and 7)
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= ((val1 ^ val2) & (val1 ^ R) & 0x80) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (val2 > val1) ? 1 : 0;  // C (Carry) flag
        sreg |= (1 & ((~val1 & val2) | (val2 & R) | (R & ~val1))) ? 0x20 : 0; // H (Half Carry) flag

        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFC00) == 0x0400) {
    // CPC opcode: 0000 01rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4;                       // Destination register index
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5);    // Source register index
        // Retrieve the values from the registers
        u8 arg1 = cpu->data[d];
        u8 arg2 = cpu->data[r];
        // Get the current status register
        u8 sreg = cpu->data[95];
        // Perform the subtraction with carry
        i16 result = arg1 - arg2 - (sreg & 1); // Carry-in is the LSB of SREG
        // Update the SREG (status register) flags
        sreg = (sreg & 0xC0) |                         // Preserve I and T flags
            ((!result && (sreg >> 1) & 1) ? 2 : 0) | // Z (Zero) flag
            ((arg2 + (sreg & 1) > arg1) ? 1 : 0);    // C (Carry) flag
        sreg |= (result & 0x80) ? 4 : 0;                // N (Negative) flag
        sreg |= ((arg1 ^ arg2) & (arg1 ^ result) & 0x80) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (1 & ((~arg1 & arg2) | (arg2 & result) | (result & ~arg1))) ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xF000) == 0x3000) {
    // CPI opcode: 0011 KKKK dddd KKKK

        // Extract the destination register (R16 to R31) and immediate value (K)
        u8 d = ((opcode & 0xF0) >> 4) + 16; // Destination register index
        u8 K = (opcode & 0xF) | ((opcode & 0xF00) >> 4); // Immediate value
        // Retrieve the value from the destination register
        u8 arg1 = cpu->data[d];
        u8 arg2 = K;
        // Perform the subtraction
        i16 r = arg1 - arg2; // Use i16 to handle potential negative results
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags (bits 6 and 7)
        sreg |= (r == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (r & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= ((arg1 ^ arg2) & (arg1 ^ r) & 0x80) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (arg2 > arg1) ? 1 : 0;  // C (Carry) flag
        sreg |= (1 & ((~arg1 & arg2) | (arg2 & r) | (r & ~arg1))) ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFC00) == 0x1000) {
    // CPSE opcode: 0001 00rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4;                       // Destination register index
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5);    // Source register index
        // Compare the register values
        if (cpu->data[d] == cpu->data[r]) {
            // Fetch the next opcode to determine the size of the next instruction
            u16 nextOpcode = cpu->programMemory[cpu->PC + 1];
            // Determine the skip size based on whether the next instruction is two words
            u8 skipSize = isTwoWordInstruction(nextOpcode) ? 2 : 1;
            // Increment the program counter to skip the appropriate number of instructions
            cpu->PC += skipSize;
            // Increment the cycle count by the number of skipped words
            cpu->cycles += skipSize;
        }
    }
    else if ((opcode & 0xFE0F) == 0x940A) {
    // DEC opcode: 1001 010d dddd 1010

        // Extract the destination register index
        u8 d = (opcode & 0x1F0) >> 4;
        // Perform the decrement operation
        u8 value = cpu->data[d];
        u8 R = value - 1;
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (value == 0x80) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if (opcode == 0x9519) {
    // EICALL opcode: 1001 0101 0001 1001

        // Calculate the return address (next instruction after EICALL)
        u32 retAddr = cpu->PC + 1;
        // Get the current stack pointer
        u16 sp = cpu->getSP();
        // Get the value of the EIND register
        u8 eind = cpu->data[0x5C];
        // Push the return address onto the stack (3 bytes for 22-bit PC)
        cpu->data[sp] = retAddr & 0xFF;             // Lower byte
        cpu->data[sp - 1] = (retAddr >> 8) & 0xFF;  // Middle byte
        cpu->data[sp - 2] = (retAddr >> 16) & 0xFF; // Upper byte
        // Update the stack pointer
        cpu->setSP(sp - 3);
        // Update the program counter to the target address
        cpu->PC = ((eind << 16) | cpu->getUint16LittleEndian(30)) - 1;
        // Increment the cycle count
        cpu->cycles += 3;
    }
    else if (opcode == 0x9419) {
    // EIJMP opcode: 1001 0100 0001 1001

        // Get the value of the EIND register
        u8 eind = cpu->data[0x5C];
        // Update the program counter to the target address
        cpu->PC = ((eind << 16) | cpu->getUint16LittleEndian(30)) - 1;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if (opcode == 0x95D8) {
    // ELPM opcode: 1001 0101 1101 1000

        // Get the value of the RAMPZ register
        u8 rampz = cpu->data[0x5B];
        // Load the byte from program memory into R0
        cpu->data[0] = cpu->programBytes[(rampz << 16) | cpu->getUint16LittleEndian(30)];
        // Increment the cycle count
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFE0F) == 0x9006) {
    // ELPM(REG) opcode: 1001 000d dddd 0110

        // Get the value of the RAMPZ register
        u8 rampz = cpu->data[0x5B];
        // Extract the destination register index (d)
        u8 d = (opcode & 0x1F0) >> 4;
        // Load the byte from program memory into the destination register
        cpu->data[d] = cpu->programBytes[(rampz << 16) | cpu->getUint16LittleEndian(30)];
        // Increment the cycle count
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFE0F) == 0x9007) {
    // ELPM(INC) opcode: 1001 000d dddd 0111

        // Get the value of the RAMPZ register
        u8 rampz = cpu->data[0x5B];
        // Get the current value of the Z register
        u16 i = cpu->getUint16LittleEndian(30);
        // Extract the destination register index (d)
        u8 d = (opcode & 0x1F0) >> 4;
        // Load the byte from program memory into the destination register
        cpu->data[d] = cpu->programBytes[(rampz << 16) | i];
        // Increment the Z register
        cpu->setUint16LittleEndian(30, i + 1);
        // Check for overflow in the Z register and update RAMPZ if needed
        if (i == 0xFFFF) {
            cpu->data[0x5B] = (rampz + 1) % (cpu->programBytes.size() >> 16);
        }
        // Increment the cycle count
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFC00) == 0x2400) {
    // EOR opcode: 0010 01rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4;                       // Destination register index
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5);    // Source register index
        // Perform the XOR operation
        u8 R = cpu->data[d] ^ cpu->data[r];
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFF88) == 0x0308) {
    // FMUL opcode: 0000 0011 0ddd 1rrr

        // Extract the source and destination register indices
        u8 d = ((opcode & 0x70) >> 4) + 16; // Destination register (R16 to R23)
        u8 r = (opcode & 0x07) + 16;       // Source register (R16 to R23)
        // Retrieve the values from the registers
        u8 v1 = cpu->data[d];
        u8 v2 = cpu->data[r];
        // Perform the fractional multiplication
        u16 R = (v1 * v2) << 1;
        // Store the result in the R1:R0 register pair
        cpu->setUint16LittleEndian(0, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xFC; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= ((v1 * v2) & 0x8000) ? 1 : 0; // C (Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    /*** XXX Might have some issues with signed stuff ***/
    else if ((opcode & 0xFF88) == 0x0380) {
    // FMULS opcode: 0000 0011 1ddd 0rrr

        // Extract the source and destination register indices
        u8 d = ((opcode & 0x70) >> 4) + 16; // Destination register (R16 to R23)
        u8 r = (opcode & 0x07) + 16;       // Source register (R16 to R23)
        // Retrieve the signed 8-bit values from the registers
        i8 v1 = static_cast<i8>(cpu->data[d]);
        i8 v2 = static_cast<i8>(cpu->data[r]);
        // Perform the signed fractional multiplication
        i16 R = (v1 * v2) << 1;
        // Store the signed 16-bit result in the R1:R0 register pair
        cpu->setInt16LittleEndian(0, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xFC; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= ((v1 * v2) & 0x8000) ? 1 : 0;   // C (Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    /*** XXX Might have some issues with signed stuff ***/
    else if ((opcode & 0xFF88) == 0x0388) {
    // FMULSU opcode: 0000 0011 1ddd 1rrr

        // Extract the source and destination register indices
        u8 d = ((opcode & 0x70) >> 4) + 16; // Destination register (R16 to R23)
        u8 r = (opcode & 0x07) + 16;       // Source register (R16 to R23)
        // Retrieve the signed and unsigned values
        i8 v1 = static_cast<i8>(cpu->data[d]); // Signed value from destination register
        u8 v2 = cpu->data[r];                 // Unsigned value from source register
        // Perform the signed/unsigned fractional multiplication
        i16 R = (v1 * v2) << 1;
        // Store the signed 16-bit result in the R1:R0 register pair
        cpu->setInt16LittleEndian(0, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xFC; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= ((v1 * v2) & 0x8000) ? 1 : 0;   // C (Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if (opcode == 0x9509) {
    // ICALL opcode: 1001 0101 0000 1001

        // Calculate the return address (next instruction after ICALL)
        u32 retAddr = cpu->PC + 1;

        // Get the current stack pointer
        u16 sp = cpu->getSP();
        // Check if the program counter supports 22 bits
        bool pc22Bits = cpu->pc22Bits;
        // Push the return address onto the stack
        cpu->data[sp] = retAddr & 0xFF;             // Lower byte
        cpu->data[sp - 1] = (retAddr >> 8) & 0xFF;  // Middle byte
        if (pc22Bits) {
            cpu->data[sp - 2] = (retAddr >> 16) & 0xFF; // High byte
        }
        // Update the stack pointer
        cpu->setSP(sp - (pc22Bits ? 3 : 2));
        // Set the program counter to the address stored in the Z register (R31:R30)
        cpu->PC = cpu->getUint16LittleEndian(30) - 1;
        // Increment the cycle count
        cpu->cycles += pc22Bits ? 3 : 2;
    }
    else if (opcode == 0x9409) {
    // IJMP opcode: 1001 0100 0000 1001

        // Set the program counter to the address in the Z register (R31:R30)
        cpu->PC = cpu->getUint16LittleEndian(30) - 1;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xF800) == 0xB000) {
    // IN opcode: 1011 0AAd dddd AAAA

        // Compute the I/O address
        u16 ioAddress = ((opcode & 0xF) | ((opcode & 0x600) >> 5)) + 32;
        // Read the value from the I/O address
        u8 i = cpu->readData(ioAddress);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4; // Destination register
        cpu->data[d] = i;
    }
    else if ((opcode & 0xFE0F) == 0x9403) {
    // INC opcode: 1001 010d dddd 0011

        // Extract the destination register index
        u8 d = (opcode & 0x1F0) >> 4;
        // Perform the increment operation
        u8 value = cpu->data[d];
        u8 result = (value + 1) & 0xFF; // Increment and wrap around if needed
        // Store the result back in the destination register
        cpu->data[d] = result;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags
        sreg |= (result == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (result & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (value == 127) ? 8 : 0;      // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFE0E) == 0x940C) {
    // JMP opcode: 1001 010k kkkk 110k kkkk kkkk kkkk kkkk

        // Compute the absolute jump target address
        u32 targetAddress = (cpu->programMemory[cpu->PC + 1]) |
                            ((opcode & 0x1) << 16) |
                            ((opcode & 0x1F0) << 13);
        // Set the program counter to the target address
        cpu->PC = targetAddress - 1;
        // Increment the cycle count for JMP
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFE0F) == 0x9206) {
    // LAC opcode: 1001 001r rrrr 0110

        // Extract the source register index
        u8 r = (opcode & 0x1F0) >> 4;
        // Retrieve the clear mask and the value from memory
        u8 clear = cpu->data[r];
        u16 address = cpu->getUint16LittleEndian(30);
        u8 value = cpu->readData(address);
        // Write the updated value back to memory (clear bits specified by the mask)
        cpu->writeData(address, value & ~clear);
        // Store the original memory value in the source register
        cpu->data[r] = value;
    }
    else if ((opcode & 0xFE0F) == 0x9205) {
    // LAS opcode: 1001 001r rrrr 0101

        // Extract the source register index
        u8 r = (opcode & 0x1F0) >> 4;
        // Retrieve the set mask and the value from memory
        u8 set = cpu->data[r];
        u16 address = cpu->getUint16LittleEndian(30);
        u8 value = cpu->readData(address);
        // Write the updated value back to memory (set bits specified by the mask)
        cpu->writeData(address, value | set);
        // Store the original memory value in the source register
        cpu->data[r] = value;
    }
    else if ((opcode & 0xFE0F) == 0x9207) {
    // LAT opcode: 1001 001r rrrr 0111

        // Extract the source register index
        u8 r = (opcode & 0x1F0) >> 4;
        // Retrieve the toggle mask and the value from memory
        u8 toggle = cpu->data[r];
        u16 address = cpu->getUint16LittleEndian(30);
        u8 value = cpu->readData(address);
        // Write the updated value back to memory (toggle bits specified by the mask)
        cpu->writeData(address, value ^ toggle);
        // Store the original memory value in the source register
        cpu->data[r] = value;
    }
    else if ((opcode & 0xF000) == 0xE000) {
    // LDI opcode: 1110 KKKK dddd KKKK

        // Compute the destination register (R16 to R31) and immediate value (K)
        u8 d = ((opcode & 0xF0) >> 4) + 16; // Destination register index
        u8 K = (opcode & 0xF) | ((opcode & 0xF00) >> 4); // Immediate value
        // Load the immediate value into the destination register
        cpu->data[d] = K;
    }
    else if ((opcode & 0xFE0F) == 0x9000) {
    // LDS opcode: 1001 000d dddd 0000 kkkk kkkk kkkk kkkk

        // Increment the cycle count for LDS
        cpu->cycles++;
        // Read the absolute address from the next word in program memory
        u16 address = cpu->programMemory[cpu->PC + 1];
        // Read the value from the specified address
        u8 value = cpu->readData(address);
        // Extract the destination register index and store the value
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
        // Increment the program counter to skip the second word of the instruction
        cpu->PC++;
    }
    else if ((opcode & 0xFE0F) == 0x900C) {
    // LDX opcode: 1001 000d dddd 1100

        // Increment the cycle count for LDX
        cpu->cycles++;
        // Read the address stored in the X register (R27:R26)
        u16 address = cpu->getUint16LittleEndian(26);
        // Read the value from the specified address
        u8 value = cpu->readData(address);
        // Extract the destination register index and store the value
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xFE0F) == 0x900D) {
    // LDX(INC) opcode: 1001 000d dddd 1101

        // Retrieve the current address stored in the X register (R27:R26)
        u16 x = cpu->getUint16LittleEndian(26);
        // Increment the cycle count
        cpu->cycles++;
        // Read the value from the address in the X register
        u8 value = cpu->readData(x);
        // Extract the destination register index and store the value
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
        // Increment the X register
        cpu->setUint16LittleEndian(26, x + 1);
    }
    else if ((opcode & 0xFE0F) == 0x900E) {
    // LDX(DEC) opcode: 1001 000d dddd 1110

        // Decrement the address stored in the X register (R27:R26)
        u16 x = cpu->getUint16LittleEndian(26) - 1;
        cpu->setUint16LittleEndian(26, x);
        // Increment the cycle count
        cpu->cycles++;
        // Read the value from the decremented address
        u8 value = cpu->readData(x);
        // Extract the destination register index and store the value
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xFE0F) == 0x8008) {
    // LDY opcode: 1000 000d dddd 1000

        // Increment the cycle count
        cpu->cycles++;
        // Load the value from the address in the Y register (R29:R28)
        u16 address = cpu->getUint16LittleEndian(28);
        u8 value = cpu->readData(address);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xFE0F) == 0x9009) {
    // LDY(INC) opcode: 1001 000d dddd 1001

        // Retrieve the current address from the Y register (R29:R28)
        u16 y = cpu->getUint16LittleEndian(28);
        // Increment the cycle count
        cpu->cycles++;
        // Read the value from the address in the Y register
        u8 value = cpu->readData(y);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
        // Increment the Y register
        cpu->setUint16LittleEndian(28, y + 1);
    }
    else if ((opcode & 0xFE0F) == 0x900A) {
    // LDY(DEC) opcode: 1001 000d dddd 1010

        // Decrement the address stored in the Y register (R29:R28)
        u16 y = cpu->getUint16LittleEndian(28) - 1;
        cpu->setUint16LittleEndian(28, y);
        // Increment the cycle count
        cpu->cycles++;
        // Read the value from the decremented address
        u8 value = cpu->readData(y);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xD208) == 0x8008 &&
         ((opcode & 0x7) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8))) {
    // LDDY opcode: 10q0 qq0d dddd 1qqq

        // Increment the cycle count
        cpu->cycles++;
        // Calculate the displacement
        u8 displacement = (opcode & 0x7) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8);
        // Compute the effective address (Y register + displacement)
        u16 address = cpu->getUint16LittleEndian(28) + displacement;
        // Read the value from the effective address
        u8 value = cpu->readData(address);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xFE0F) == 0x8000) {
    // LDZ opcode: 1000 000d dddd 0000

        // Increment the cycle count
        cpu->cycles++;
        // Load the value from the address in the Z register (R31:R30)
        u16 address = cpu->getUint16LittleEndian(30);
        u8 value = cpu->readData(address);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xFE0F) == 0x9001) {
    // LDZ(INC) opcode: 1001 000d dddd 0001

        // Retrieve the current address from the Z register (R31:R30)
        u16 z = cpu->getUint16LittleEndian(30);
        // Increment the cycle count
        cpu->cycles++;
        // Read the value from the address in the Z register
        u8 value = cpu->readData(z);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
        // Increment the Z register
        cpu->setUint16LittleEndian(30, z + 1);
    }
    else if ((opcode & 0xFE0F) == 0x9002) {
    // LDZ(DEC) opcode: 1001 000d dddd 0010

        // Decrement the address stored in the Z register (R31:R30)
        u16 z = cpu->getUint16LittleEndian(30) - 1;
        cpu->setUint16LittleEndian(30, z);
        // Increment the cycle count
        cpu->cycles++;
        // Read the value from the decremented address
        u8 value = cpu->readData(z);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if ((opcode & 0xD208) == 0x8000 &&
         ((opcode & 0x7) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8))) {
    // LDDZ opcode: 10q0 qq0d dddd 0qqq

        // Increment the cycle count
        cpu->cycles++;
        // Calculate the displacement
        u8 displacement = (opcode & 0x7) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8);
        // Compute the effective address (Z register + displacement)
        u16 address = cpu->getUint16LittleEndian(30) + displacement;
        // Read the value from the effective address
        u8 value = cpu->readData(address);
        // Store the value in the destination register
        u8 d = (opcode & 0x1F0) >> 4;
        cpu->data[d] = value;
    }
    else if (opcode == 0x95C8) {
    // LPM opcode: 1001 0101 1100 1000

        // Load the byte from program memory into R0
        u16 address = cpu->getUint16LittleEndian(30); // Address in the Z register
        cpu->data[0] = cpu->programBytes[address];
        // Increment the cycle count
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFE0F) == 0x9004) {
    // LPM(REG) opcode: 1001 000d dddd 0100

        // Load the byte from program memory into the destination register
        u16 address = cpu->getUint16LittleEndian(30); // Address in the Z register
        u8 d = (opcode & 0x1F0) >> 4; // Destination register
        cpu->data[d] = cpu->programBytes[address];
        // Increment the cycle count
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFE0F) == 0x9005) {
    // LPM(INC) opcode: 1001 000d dddd 0101

        // Retrieve the current address from the Z register
        u16 address = cpu->getUint16LittleEndian(30);
        // Load the byte from program memory into the destination register
        u8 d = (opcode & 0x1F0) >> 4; // Destination register
        cpu->data[d] = cpu->programBytes[address];
        // Increment the Z register
        cpu->setUint16LittleEndian(30, address + 1);
        // Increment the cycle count
        cpu->cycles += 2;
    }
    else if ((opcode & 0xFE0F) == 0x9406) {
    // LSR opcode: 1001 010d dddd 0110

        // Extract the destination register index and its value
        u8 d = (opcode & 0x1F0) >> 4;
        u8 value = cpu->data[d];
        // Perform the logical shift right operation
        u8 R = value >> 1;
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE0; // Preserve I, T, and H flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (value & 1);            // C (Carry) flag
        sreg |= (((sreg >> 2) & 1) ^ (sreg & 1)) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFC00) == 0x2C00) {
    // MOV opcode: 0010 11rd dddd rrrr
        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4; // Destination register
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5); // Source register
        // Move the value from the source register to the destination register
        cpu->data[d] = cpu->data[r];
    }
    else if ((opcode & 0xFF00) == 0x0100) {
    // MOVW opcode: 0000 0001 dddd rrrr

        // Extract the source (r) and destination (d) register pairs
        u8 r2 = 2 * (opcode & 0xF); // Source register pair
        u8 d2 = 2 * ((opcode & 0xF0) >> 4); // Destination register pair
        // Move the word (2 bytes) from the source register pair to the destination register pair
        cpu->data[d2] = cpu->data[r2];       // Lower byte
        cpu->data[d2 + 1] = cpu->data[r2 + 1]; // Upper byte
    }
    else if ((opcode & 0xFC00) == 0x9C00) {
    // MUL opcode: 1001 11rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4; // Destination register
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5); // Source register
        // Perform the multiplication
        u16 R = cpu->data[d] * cpu->data[r];
        // Store the 16-bit result in the R1:R0 register pair
        cpu->setUint16LittleEndian(0, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xFC; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x8000) ? 1 : 0;   // C (Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    /*** XXX Might have some issues with signed stuff ***/
    else if ((opcode & 0xFF00) == 0x0200) {
    // MULS opcode: 0000 0010 dddd rrrr

        // Extract the source and destination register indices
        u8 d = ((opcode & 0xF0) >> 4) + 16; // Destination register (R16 to R31)
        u8 r = (opcode & 0xF) + 16;        // Source register (R16 to R31)
        // Retrieve the signed values from the registers
        i8 v1 = static_cast<i8>(cpu->data[d]);
        i8 v2 = static_cast<i8>(cpu->data[r]);
        // Perform the signed multiplication
        i16 R = v1 * v2;
        // Store the signed 16-bit result in the R1:R0 register pair
        cpu->setInt16LittleEndian(0, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xFC; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x8000) ? 1 : 0;   // C (Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;

        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFF88) == 0x0300) {
    // MULSU opcode: 0000 0011 0ddd 0rrr

        // Extract the source and destination register indices
        u8 d = ((opcode & 0x70) >> 4) + 16; // Destination register (R16 to R23)
        u8 r = (opcode & 0x7) + 16;        // Source register (R16 to R23)
        // Retrieve the signed and unsigned values
        i8 v1 = static_cast<i8>(cpu->data[d]); // Signed value from destination register
        u8 v2 = cpu->data[r];                  // Unsigned value from source register
        // Perform the signed/unsigned multiplication
        i16 R = v1 * v2;
        // Store the signed 16-bit result in the R1:R0 register pair
        cpu->setInt16LittleEndian(0, R);
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xFC; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x8000) ? 1 : 0;   // C (Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x9401) {
    // NEG opcode: 1001 010d dddd 0001

        // Extract the destination register index
        u8 d = (opcode & 0x1F0) >> 4;
        // Perform the two's complement operation
        u8 value = cpu->data[d];
        u8 R = 0 - value;
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (R == 0x80) ? 8 : 0;    // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (R != 0) ? 1 : 0;       // C (Carry) flag
        sreg |= ((R | value) & 0x1) ? 0x20 : 0; // H (Half Carry) flag

        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if (opcode == 0x0000) {
    // NOP opcode: 0000 0000 0000 0000
        // NOP
    }
    else if ((opcode & 0xFC00) == 0x2800) {
    // OR opcode: 0010 10rd dddd rrrr

        // Perform the logical OR operation
        u8 d = (opcode & 0x1F0) >> 4; // Destination register index
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5); // Source register index
        u8 R = cpu->data[d] | cpu->data[r];
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xF000) == 0x6000) {
    // SBR opcode: 0110 KKKK dddd KKKK

        // Perform the bitwise OR operation
        u8 d = ((opcode & 0xF0) >> 4) + 16; // Destination register (R16 to R31)
        u8 K = (opcode & 0xF) | ((opcode & 0xF00) >> 4); // Immediate value
        u8 R = cpu->data[d] | K;
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE1; // Preserve I, T, and C flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xF800) == 0xB800) {
    // OUT opcode: 1011 1AAr rrrr AAAA

        // Compute the I/O address
        u16 ioAddress = ((opcode & 0xF) | ((opcode & 0x600) >> 5)) + 32;
        // Write the value from the source register to the I/O address
        u8 r = (opcode & 0x1F0) >> 4; // Source register
        cpu->writeData(ioAddress, cpu->data[r]);
    }
    else if ((opcode & 0xFE0F) == 0x900F) {
    // POP opcode: 1001 000d dddd 1111

        // Increment the stack pointer
        u16 sp = cpu->getUint16LittleEndian(93) + 1;
        cpu->setUint16LittleEndian(93, sp);
        // Load the value from the stack into the destination register
        u8 d = (opcode & 0x1F0) >> 4; // Destination register
        cpu->data[d] = cpu->data[sp];
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x920F) {
    // PUSH opcode: 1001 001d dddd 1111

        // Get the current stack pointer
        u16 sp = cpu->getUint16LittleEndian(93);
        // Retrieve the value from the source register
        u8 d = (opcode & 0x1F0) >> 4; // Source register index
        u8 value = cpu->data[d];
        // Push the value onto the stack
        cpu->data[sp] = value;
        // Decrement the stack pointer
        cpu->setUint16LittleEndian(93, sp - 1);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xF000) == 0xD000) {
    // RCALL opcode: 1101 kkkk kkkk kkkk

        // Compute the relative offset (k)
        i16 k = (opcode & 0x7FF) - ((opcode & 0x800) ? 0x800 : 0);
        // Compute the return address (next instruction)
        u32 retAddr = cpu->PC + 1;
        // Get the current stack pointer
        u16 sp = cpu->getUint16LittleEndian(93);
        // Check if program counter supports 22 bits
        bool pc22Bits = cpu->pc22Bits;
        // Push the return address onto the stack
        cpu->data[sp] = retAddr & 0xFF;             // Lower byte
        cpu->data[sp - 1] = (retAddr >> 8) & 0xFF;  // Middle byte
        if (pc22Bits) {
            cpu->data[sp - 2] = (retAddr >> 16) & 0xFF; // High byte
        }
        // Update the stack pointer
        cpu->setUint16LittleEndian(93, sp - (pc22Bits ? 3 : 2));
        // Update the program counter
        cpu->PC += k;
        // Increment the cycle count
        cpu->cycles += pc22Bits ? 3 : 2;
    }
    else if (opcode == 0x9508) {
    // RET opcode: 1001 0101 0000 1000

        // Check if program counter supports 22 bits
        bool pc22Bits = cpu->pc22Bits;
        // Get the current stack pointer and adjust for return address size
        u16 sp = cpu->getUint16LittleEndian(93) + (pc22Bits ? 3 : 2);
        cpu->setUint16LittleEndian(93, sp);
        // Retrieve the return address from the stack
        u32 retAddr = (cpu->data[sp - 1] << 8) | cpu->data[sp];
        if (pc22Bits) {
            retAddr |= (cpu->data[sp - 2] << 16);
        }
        // Set the program counter to the return address
        cpu->PC = retAddr - 1;
        // Increment the cycle count
        cpu->cycles += pc22Bits ? 4 : 3;
    }
    else if (opcode == 0x9518) {
    // RETI opcode: 1001 0101 0001 1000

        // Check if program counter supports 22 bits
        bool pc22Bits = cpu->pc22Bits;
        // Get the current stack pointer and adjust for return address size
        u16 sp = cpu->getUint16LittleEndian(93) + (pc22Bits ? 3 : 2);
        cpu->setUint16LittleEndian(93, sp);

        // Retrieve the return address from the stack
        u32 retAddr = (cpu->data[sp - 1] << 8) | cpu->data[sp];
        if (pc22Bits) {
            retAddr |= (cpu->data[sp - 2] << 16);
        }

        // Set the program counter to the return address
        cpu->PC = retAddr - 1;

        // Enable interrupts
        cpu->data[95] |= 0x80;

        // Increment the cycle count
        cpu->cycles += pc22Bits ? 4 : 3;
    }
    /*** XXX Might have some issues with signed stuff ***/
    else if ((opcode & 0xF000) == 0xC000) {
    // RJMP opcode: 1100 kkkk kkkk kkkk

        // Compute the relative offset (k)
        i16 k = (opcode & 0x7FF) - ((opcode & 0x800) ? 0x800 : 0);
        // Update the program counter
        cpu->PC += k;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x9407) {
    // ROR opcode: 1001 010d dddd 0111

        // Extract the destination register index and value
        u8 d = (opcode & 0x1F0) >> 4;
        u8 value = cpu->data[d];
        // Perform the rotate right operation through the carry flag
        u8 carry = cpu->data[95] & 0x01; // Carry flag (bit 0 of SREG)
        u8 R = (value >> 1) | (carry << 7);
        // Store the result back in the destination register
        cpu->data[d] = R;

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xE0; // Preserve I, T, and H flags
        sreg |= (R == 0) ? 2 : 0;       // Z (Zero) flag
        sreg |= (R & 0x80) ? 4 : 0;     // N (Negative) flag
        sreg |= (value & 0x01) ? 1 : 0; // C (Carry) flag
        sreg |= (((sreg >> 2) & 1) ^ (sreg & 1)) ? 8 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFC00) == 0x0800) {
    // SBC opcode: 0000 10rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4;                       // Destination register
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5);    // Source register
        // Retrieve the values from the registers
        u8 val1 = cpu->data[d];
        u8 val2 = cpu->data[r];
        // Get the carry bit from the SREG
        u8 carry = cpu->data[95] & 0x01;
        // Perform the subtraction with carry
        i16 R = val1 - val2 - carry;
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95];
        sreg = (sreg & 0xC0) | (!R && ((sreg >> 1) & 1) ? 0x02 : 0); // Z (Zero) flag
        sreg |= (val2 + carry > val1) ? 0x01 : 0;                    // C (Carry) flag
        sreg |= (R & 0x80) ? 0x04 : 0;                               // N (Negative) flag
        sreg |= ((val1 ^ val2) & (val1 ^ R) & 0x80) ? 0x08 : 0;       // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0;   // S (Sign) flag
        sreg |= (1 & ((~val1 & val2) | (val2 & R) | (R & ~val1))) ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
   else if ((opcode & 0xF000) == 0x4000) {
    // SBCI opcode: 0100 KKKK dddd KKKK

        // Extract the destination register and immediate value
        u8 d = ((opcode & 0xF0) >> 4) + 16; // Destination register (R16 to R31)
        u8 val2 = (opcode & 0xF) | ((opcode & 0xF00) >> 4); // Immediate value
        // Retrieve the value from the destination register
        u8 val1 = cpu->data[d];
        // Get the carry bit from the SREG
        u8 carry = cpu->data[95] & 0x01;
        // Perform the subtraction with carry
        i16 R = val1 - val2 - carry;
        // Store the result back in the destination register
        cpu->data[d] = R;
        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95];
        sreg = (sreg & 0xC0) | (!R && ((sreg >> 1) & 1) ? 0x02 : 0); // Z (Zero) flag
        sreg |= (val2 + carry > val1) ? 0x01 : 0;                    // C (Carry) flag
        sreg |= (R & 0x80) ? 0x04 : 0;                               // N (Negative) flag
        sreg |= ((val1 ^ val2) & (val1 ^ R) & 0x80) ? 0x08 : 0;       // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0;   // S (Sign) flag
        sreg |= ((~val1 & val2) | (val2 & R) | (R & ~val1)) & 0x08 ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFF00) == 0x9A00) {
    // SBI opcode: 1001 1010 AAAA Abbb

        // Extract the I/O address and bit mask
        u8 address = ((opcode & 0xF8) >> 3) + 32;
        u8 mask = 1 << (opcode & 0x07);
        // Set the specified bit in the I/O register
        u8 value = cpu->readData(address);
        cpu->writeData(address, value | mask, mask);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFF00) == 0x9900) {
    // SBIC opcode: 1001 1001 AAAA Abbb

        // Compute the I/O address and bit mask
        u8 address = ((opcode & 0xF8) >> 3) + 32;
        u8 mask = 1 << (opcode & 0x07);
        // Read the value from the I/O register
        u8 value = cpu->readData(address);
        // Check if the bit is cleared
        if (!(value & mask)) {
            // Fetch the next opcode and determine the skip size
            u16 nextOpcode = cpu->programMemory[cpu->PC + 1];
            u8 skipSize = isTwoWordInstruction(nextOpcode) ? 2 : 1;
            // Skip the next instruction(s)
            cpu->cycles += skipSize;
            cpu->PC += skipSize;
        }
    }
    else if ((opcode & 0xFF00) == 0x9B00) {
    // SBIS opcode: 1001 1011 AAAA Abbb

        // Compute the I/O address and bit mask
        u8 address = ((opcode & 0xF8) >> 3) + 32;
        u8 mask = 1 << (opcode & 0x07);
        // Read the value from the I/O register
        u8 value = cpu->readData(address);
        // Check if the bit is set
        if (value & mask) {
            // Fetch the next opcode and determine the skip size
            u16 nextOpcode = cpu->programMemory[cpu->PC + 1];
            u8 skipSize = isTwoWordInstruction(nextOpcode) ? 2 : 1;
            // Skip the next instruction(s)
            cpu->cycles += skipSize;
            cpu->PC += skipSize;
        }
    }
    else if ((opcode & 0xFF00) == 0x9700) {
    // SBIW opcode: 1001 0111 KKdd KKKK

        // Compute the register pair and immediate value
        u16 regAddress = 2 * ((opcode & 0x30) >> 4) + 24; // Address of the register pair (R25:R24, etc.)
        u16 regValue = cpu->getUint16LittleEndian(regAddress); // Retrieve the current value
        u8 K = (opcode & 0x0F) | ((opcode & 0xC0) >> 2); // Immediate value
        // Perform the subtraction
        u16 R = regValue - K;
        // Store the result back in the register pair
        cpu->setUint16LittleEndian(regAddress, R);

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags
        sreg |= (R == 0) ? 0x02 : 0;    // Z (Zero) flag
        sreg |= (R & 0x8000) ? 0x04 : 0; // N (Negative) flag
        sreg |= ((regValue & ~R) & 0x8000) ? 0x08 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (K > regValue) ? 0x01 : 0;  // C (Carry) flag
        sreg |= (1 & ((~regValue & K) | (K & R) | (R & ~regValue))) ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE08) == 0xFC00) {
    // SBRC opcode: 1111 110r rrrr 0bbb

        // Extract the source register and bit mask
        u8 r = (opcode & 0x1F0) >> 4; // Source register index
        u8 mask = 1 << (opcode & 0x07); // Bit position mask
        // Check if the specified bit is cleared
        if (!(cpu->data[r] & mask)) {
            // Fetch the next opcode and determine the skip size
            u16 nextOpcode = cpu->programMemory[cpu->PC + 1];
            u8 skipSize = isTwoWordInstruction(nextOpcode) ? 2 : 1;
            // Skip the next instruction(s)
            cpu->cycles += skipSize;
            cpu->PC += skipSize;
        }
    }
    else if ((opcode & 0xFE08) == 0xFE00) {
    // SBRS opcode: 1111 111r rrrr 0bbb

        // Extract the source register and bit mask
        u8 r = (opcode & 0x1F0) >> 4; // Source register index
        u8 mask = 1 << (opcode & 0x07); // Bit position mask

        // Check if the specified bit is set
        if (cpu->data[r] & mask) {
            // Fetch the next opcode and determine the skip size
            u16 nextOpcode = cpu->programMemory[cpu->PC + 1];
            u8 skipSize = isTwoWordInstruction(nextOpcode) ? 2 : 1;
            // Skip the next instruction(s)
            cpu->cycles += skipSize;
            cpu->PC += skipSize;
        }
    } 
    else if (opcode == 0x9588) {
    /* SLEEP, 1001 0101 1000 1000 */
    /* not implemented */
    } 
    else if (opcode == 0x95e8) {
    /* SPM, 1001 0101 1110 1000 */
    /* not implemented */
    } 
    else if (opcode == 0x95f8) {
    /* SPM(INC), 1001 0101 1111 1000 */
    /* not implemented */
    }
    else if ((opcode & 0xFE0F) == 0x9200) {
    // STS opcode: 1001 001d dddd 0000 kkkk kkkk kkkk kkkk

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Fetch the address from the next word in program memory
        u16 address = cpu->programMemory[cpu->PC + 1];

        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the program counter to skip the second word
        cpu->PC++;
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x920C) {
    // STX opcode: 1001 001r rrrr 1100

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Retrieve the address from the X register (R27:R26)
        u16 address = cpu->getUint16LittleEndian(26);
        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x920D) {
    // STX(INC) opcode: 1001 001r rrrr 1101

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Retrieve the address from the X register (R27:R26)
        u16 address = cpu->getUint16LittleEndian(26);
        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the X register
        cpu->setUint16LittleEndian(26, address + 1);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x920E) {
    // STX(DEC) opcode: 1001 001r rrrr 1110

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Decrement the X register
        u16 address = cpu->getUint16LittleEndian(26) - 1;
        cpu->setUint16LittleEndian(26, address);
        // Write the value to the decremented address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x8208) {
    // STY opcode: 1000 001r rrrr 1000

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Retrieve the address from the Y register (R29:R28)
        u16 address = cpu->getUint16LittleEndian(28);
        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x9209) {
    // STY(INC) opcode: 1001 001r rrrr 1001

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Retrieve the address from the Y register (R29:R28)
        u16 address = cpu->getUint16LittleEndian(28);
        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the Y register
        cpu->setUint16LittleEndian(28, address + 1);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x920A) {
    // STY(DEC) opcode: 1001 001r rrrr 1010

        // Retrieve the value from the destination register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Decrement the Y register
        u16 address = cpu->getUint16LittleEndian(28) - 1;
        cpu->setUint16LittleEndian(28, address);
        // Write the value to the decremented address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xD208) == 0x8208 &&
         ((opcode & 0x7) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8))) {
    // STDY opcode: 10q0 qq1r rrrr 1qqq

        // Compute the displacement
        u8 displacement = (opcode & 0x07) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8);
        // Retrieve the base address from the Y register (R29:R28)
        u16 baseAddress = cpu->getUint16LittleEndian(28);
        // Compute the effective address
        u16 address = baseAddress + displacement;
        // Retrieve the value from the source register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Write the value to the effective address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x8200) {
    // STZ opcode: 1000 001r rrrr 0000

        // Retrieve the value from the source register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Retrieve the address from the Z register (R31:R30)
        u16 address = cpu->getUint16LittleEndian(30);
        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x9201) {
    // STZ(INC) opcode: 1001 001r rrrr 0001

        // Retrieve the value from the source register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Retrieve the address from the Z register (R31:R30)
        u16 address = cpu->getUint16LittleEndian(30);
        // Write the value to the specified address
        cpu->writeData(address, value);
        // Increment the Z register
        cpu->setUint16LittleEndian(30, address + 1);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFE0F) == 0x9202) {
    // STZ(DEC) opcode: 1001 001r rrrr 0010

        // Retrieve the value from the source register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Decrement the Z register
        u16 address = cpu->getUint16LittleEndian(30) - 1;
        cpu->setUint16LittleEndian(30, address);
        // Write the value to the decremented address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xD208) == 0x8200 &&
         ((opcode & 0x7) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8))) {
    // STDZ opcode: 10q0 qq1r rrrr 0qqq

        // Compute the displacement
        u8 displacement = (opcode & 0x07) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8);
        // Retrieve the base address from the Z register (R31:R30)
        u16 baseAddress = cpu->getUint16LittleEndian(30);
        // Compute the effective address
        u16 address = baseAddress + displacement;
        // Retrieve the value from the source register
        u8 value = cpu->data[(opcode & 0x1F0) >> 4];
        // Write the value to the effective address
        cpu->writeData(address, value);
        // Increment the cycle count
        cpu->cycles++;
    }
    else if ((opcode & 0xFC00) == 0x1800) {
    // SUB opcode: 0001 10rd dddd rrrr

        // Extract the source (r) and destination (d) registers
        u8 d = (opcode & 0x1F0) >> 4;                       // Destination register
        u8 r = (opcode & 0xF) | ((opcode & 0x200) >> 5);    // Source register
        // Retrieve the values from the registers
        u8 val1 = cpu->data[d];
        u8 val2 = cpu->data[r];
        // Perform the subtraction
        i16 R = val1 - val2;
        // Store the result back in the destination register
        cpu->data[d] = R;

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags
        sreg |= (R == 0) ? 0x02 : 0;    // Z (Zero) flag
        sreg |= (R & 0x80) ? 0x04 : 0;  // N (Negative) flag
        sreg |= ((val1 ^ val2) & (val1 ^ R) & 0x80) ? 0x08 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (val2 > val1) ? 0x01 : 0; // C (Carry) flag
        sreg |= (1 & ((~val1 & val2) | (val2 & R) | (R & ~val1))) ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xF000) == 0x5000) {
    // SUBI opcode: 0101 KKKK dddd KKKK

        // Extract the destination register and immediate value
        u8 d = ((opcode & 0xF0) >> 4) + 16; // Destination register (R16 to R31)
        u8 val2 = (opcode & 0xF) | ((opcode & 0xF00) >> 4); // Immediate value
        // Retrieve the value from the destination register
        u8 val1 = cpu->data[d];
        // Perform the subtraction
        i16 R = val1 - val2;

        // Store the result back in the destination register
        cpu->data[d] = R;

        // Update the SREG (status register) flags
        u8 sreg = cpu->data[95] & 0xC0; // Preserve I and T flags
        sreg |= (R == 0) ? 0x02 : 0;    // Z (Zero) flag
        sreg |= (R & 0x80) ? 0x04 : 0;  // N (Negative) flag
        sreg |= ((val1 ^ val2) & (val1 ^ R) & 0x80) ? 0x08 : 0; // V (Overflow) flag
        sreg |= (((sreg >> 2) & 1) ^ ((sreg >> 3) & 1)) ? 0x10 : 0; // S (Sign) flag
        sreg |= (val2 > val1) ? 0x01 : 0; // C (Carry) flag
        sreg |= (1 & ((~val1 & val2) | (val2 & R) | (R & ~val1))) ? 0x20 : 0; // H (Half Carry) flag
        // Write the updated SREG back to the CPU
        cpu->data[95] = sreg;
    }
    else if ((opcode & 0xFE0F) == 0x9402) {
    // SWAP opcode: 1001 010d dddd 0010

        // Extract the destination register index
        u8 d = (opcode & 0x1F0) >> 4;
        // Retrieve the value from the destination register
        u8 value = cpu->data[d];
        // Swap the nibbles (high and low 4 bits)
        cpu->data[d] = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
    }
    else if (opcode == 0x95A8) {
    // WDR opcode: 1001 0101 1010 1000

        // Trigger the watchdog reset operation
        cpu->onWatchdogReset();
    }
    else if ((opcode & 0xFE0F) == 0x9204) {
        // XCH opcode: 1001 001r rrrr 0100

        // Extract the source register index
        u8 r = (opcode & 0x1F0) >> 4;
        // Retrieve the address from the Z register (R31:R30)
        u16 address = cpu->getUint16LittleEndian(30);
        // Exchange values between the register and the memory location
        u8 val1 = cpu->data[r];
        u8 val2 = cpu->data[address];
        // Perform the exchange
        cpu->data[address] = val1;
        cpu->data[r] = val2;
    }
    cpu->PC = (cpu->PC + 1) % cpu->programMemory.size();
    cpu->cycles++;
}

/*** TESTING ZONE ***/


// General-purpose registers
constexpr int r0 = 0;
constexpr int r1 = 1;
constexpr int r2 = 2;
constexpr int r3 = 3;
constexpr int r4 = 4;
constexpr int r5 = 5;
constexpr int r6 = 6;
constexpr int r7 = 7;
constexpr int r8 = 8;
constexpr int r16 = 16;
constexpr int r17 = 17;
constexpr int r18 = 18;
constexpr int r19 = 19;
constexpr int r20 = 20;
constexpr int r21 = 21;
constexpr int r22 = 22;
constexpr int r23 = 23;
constexpr int r24 = 24;
constexpr int r26 = 26;
constexpr int r27 = 27;
constexpr int r31 = 31;

// Pointer registers
constexpr int X = 26;
constexpr int Y = 28;
constexpr int Z = 30;

// Special registers
constexpr int RAMPZ = 0x5B;
constexpr int EIND = 0x5C;
constexpr int SP = 93;
constexpr int SPH = 94;
constexpr int SREG = 95;

// SREG Bits: I-HSVNZC
constexpr int SREG_C = 0b00000001; // Carry
constexpr int SREG_Z = 0b00000010; // Zero
constexpr int SREG_N = 0b00000100; // Negative
constexpr int SREG_V = 0b00001000; // Overflow
constexpr int SREG_S = 0b00010000; // Sign
constexpr int SREG_H = 0b00100000; // Half Carry
constexpr int SREG_I = 0b10000000; // Global Interrupt Enable

int main(){

    /* SBC  ✅*/
    /*
    it('should execute `SBC r0, r1` instruction when carry is on and result overflows', () => {
    loadProgram('SBC r0, r1');
    cpu.data[r0] = 0;
    cpu.data[r1] = 10;
    cpu.data[95] = SREG_C;
    avrInstruction(cpu);
    expect(cpu.pc).toEqual(1);
    expect(cpu.cycles).toEqual(1);
    expect(cpu.data[r0]).toEqual(245);
    expect(cpu.data[SREG]).toEqual(SREG_H | SREG_S | SREG_N | SREG_C);
  });
  */
    // std::vector<u16> testPM(0x8000);
    // CPU *cpu = new CPU(testPM);
    // // Manually load into program memory
    // //...
    // cpu->programMemory[0] = 0x0801;
    // cpu->data[r0] = 0;
    // cpu->data[r1] = 10;
    // cpu->data[95] = SREG_C;
    // avrInstruction(cpu);
    // std::cout << "expect(cpu.pc).toEqual(1): " << int(cpu->PC) << std::endl;
    // std::cout << "expect(cpu.cycles).toEqual(1): " << int(cpu->cycles) << std::endl;
    // std::cout << "expect(cpu.data[r0]).toEqual(245): " << int(cpu->data[r0]) << std::endl;
    // std::cout << "expect(cpu.data[SREG]).toEqual(SREG_H | SREG_S | SREG_N | SREG_C): " << int(cpu->data[SREG]) << std::endl;




}