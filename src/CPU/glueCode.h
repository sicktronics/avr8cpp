#include "CPU.h"
#pragma once

class AVRBoard {

    public:

    // program memory
    std::vector<u16> FLASH = std::vector<u16>(0x8000);

    // The cpu
    CPU *cpu = new CPU(FLASH);

    // bools for needToQueueInterrupt, simActive, interruptCleared
    bool needToQueueInterrupt = false;
    bool simActive = false;
    bool interruptCleared = false;

    // Execute function which calls "tick" while simActive is true
    void execute();

};