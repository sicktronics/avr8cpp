#include "index.h"

#pragma once

// ATMega 328p
const u16 FLASH = 0x8000;
const int workUnitCycles = 5000000;

class AVRRunner {
    public:
    std::vector<u16> program;
    std::vector<u8> programB;
    CPU *cpu;
    AVRTimer *timer0;
    // AVRTimer *timer1;
    // AVRTimer *timer2;
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

