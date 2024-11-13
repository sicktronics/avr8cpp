#include "CPU.h"
#include "interrupt.h"
#include <iostream>

int main(int argc, char* argv[]) {



// ✅ TEST 1: Confirm it works with 16-bit addressing
    // // Create a new CPU with program memory of 0x8000
    // std::vector<u16> testPM(0x8000);
    // CPU *cpu = new CPU(testPM);
    // // Point the PC to some rAnDoM location
    // cpu->PC = 0x520;
    // // Set SP to a random place on the stack
    // cpu->data[94] = 0;
    // cpu->data[93] = 0x80; // SP <- 0x80
    // // Enable global interrupts...and also the carry flag (why? just for testing - see below)
    // cpu->data[95] = 0b10000001; // SREG <- I------C
    // // trigger an interrupt with the current cpu and address of 5
    // avrInterrupt(cpu, 5);
    // // Once it returns, check number of cycles (should be 2)
    // std::cout << "Number of cycles should = 2: " << cpu->cycles << std::endl;
    // // PC should be the address we passed in
    // std::cout << "PC should be 5: " << cpu->PC << std::endl;
    // // SP should point to its old location -2  
    // std::cout << "SP should be 0x7e = 126: " << int(cpu->data[93]) << std::endl;
    // // old PC address should be properly stored on the stack
    // std::cout << "LSB of PC (0x20) should be stored at position 0x80: " << int(cpu->data[0x80]) << std::endl;
    // std::cout << "MSB of PC (0x5) should be stored at position 0x7F: " << int(cpu->data[0x7F]) << std::endl;
    // // Global interupts should be disabled
    // std::cout << "SREG should just be 1 (interrupts enabled cleared): " << int(cpu->data[95]) << std::endl;

// ✅ TEST 2: Confirm it works with 22-bit addressing
    // // Create a cpu with a big honkin' progMem
    // std::vector<u16> testPM(0x80000);
    // CPU *cpu = new CPU(testPM);
    // // std::cout << int(cpu->pc22Bits) << std::endl;
    // // double check that 22-bit addressing is triggered
    // std::cout << "22-bit addressing should be 1/true: " << cpu->pc22Bits << std::endl;
    // // Point pc to random location
    // cpu->PC = 0x10520;
    // // Set up SP at random location
    // cpu->data[94] = 0;
    // cpu->data[93] = 0x80; // SP <- 0x80
    // // Set SREG to enable interrupts and carry (for testing)
    // cpu->data[95] = 0b10000001; // SREG <- I------C
    // // trigger an interrupt with the current cpu and address of 5
    // avrInterrupt(cpu, 5);
    // // Once it returns, check number of cycles (should be 2)
    // std::cout << "Number of cycles should = 2: " << cpu->cycles << std::endl;
    // // PC should be the address we passed in
    // std::cout << "PC should be 5: " << cpu->PC << std::endl;
    // // SP should decrement by 3
    // std::cout << "SP should be 0x7d = 125: " << int(cpu->data[93]) << std::endl;
    // // old PC address should be properly stored on the stack (22-bits wide)
    // std::cout << "LSB of PC (0x20) should be stored at position 0x80: " << int(cpu->data[0x80]) << std::endl;
    // std::cout << "2nd MSB of PC (0x05) should be stored at position 0x7F: " << int(cpu->data[0x7F]) << std::endl;
    // std::cout << "MSB of PC (0x1) should be stored at position 0x7E: " << int(cpu->data[0x7E]) << std::endl;
    // // Confirm global interrupts CLEARED
    // std::cout << "SREG should just be 1 (interrupts enabled cleared): " << int(cpu->data[95]) << std::endl;


// ✅ TEST 3: Test the tick() function with interrupt handling - no restore
    // // Create a new CPU with program memory of 0x8000
    // std::vector<u16> testPM(0x8000);
    // CPU *cpu = new CPU(testPM);
    // // Enable global interrupts
    // cpu->data[95] = 0b10000000;
    // // Set up an interrupt, set constant = false, give it an address, add it to the queue
    // AVRInterruptConfig *testInt = new AVRInterruptConfig;
    // testInt->constant = false;
    // testInt->address = 19;
    // cpu->queueInterrupt(testInt);
    // // Call tick()
    // cpu->tick();
    // // Confirm SP pointing to new top (-2)
    // std::cout << "SP should be 8847-2 = 8845: " << int(cpu->getSP()) << std::endl;
    // // confirm PC pointing to interrupt address
    // std::cout << "PC should be interrupt address, 19: " << int(cpu->PC) << std::endl;
    // // CPU cycles should be 2
    // std::cout << "CPU cycles: " << int(cpu->cycles) << std::endl;

}