// #include "index.h" // Internal testing
#include "intelHex.h"
// #include <unistd.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include "../CPU/CPU.h"
#include "../CPU/instruction.h"
#include "../peripherals/GPIO.h"
#include "../peripherals/timer.h"

#pragma once

extern bool simBeginTicking;

// ATMega 328p
const u16 FLASH = 0x8000;
const int workUnitCycles = 500000;

class AVRRunner {
    public:

    // bool simBeginTicking = false;

    std::vector<u16> program;
    std::vector<u8> programB;
    CPU *cpu;
    AVRTimer *timer0;
    AVRTimer *timer1;
    AVRTimer *timer2;
    AVRIOPort *portB;
    AVRIOPort *portC;    
    AVRIOPort *portD;
    // Usart
    // Other stuff

    AVRRunner(std::string lilHexGal);

    // Skipping task scheduler stuff for now...

    // void execute(CPU *execCPU);
    void execute();

};

