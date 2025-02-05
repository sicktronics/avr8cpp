#include "execute.h"
#include "index.h"
// #pragma once

bool simBeginTicking = false;

AVRRunner::AVRRunner(std::string lilHexGal){

    simBeginTicking = false;

    // First things first, we need to call loadHex() to load the hex into program memory
    std::vector<u16> program = std::vector<u16>(0);
    program.reserve(FLASH);
    // program.reserve(FLASH);
    std::vector<u8> programB = std::vector<u8>(0);
    // std::cout << programB.size() << std::endl;
    loadHex(lilHexGal, programB);

    // Now, we need to fill the u16 array with the u8 array contents
    for (size_t i = 0; i < programB.size(); i += 2) {
        if (programB.size() % 2 != 0) {
            throw std::runtime_error("programB size is not even.");
        }
        u16 value = programB[i] | programB[i + 1] << 8;
        // std::cout << value << std::endl;
        this->program.push_back(value);
    }

    cpu = new CPU(this->program);
    this->timer0 = new AVRTimer(this->cpu, new timer0Config);
    this->timer1 = new AVRTimer(this->cpu, new timer1Config);
    this->timer2 = new AVRTimer(this->cpu, new timer2Config);
    this->portB = new AVRIOPort(this->cpu, new portBConfig);
    this->portC = new AVRIOPort(this->cpu, new portCConfig);
    this->portD = new AVRIOPort(this->cpu, new portDConfig);

    // Usart
    // Skip task scheduler
}

void AVRRunner::execute(){
    simBeginTicking = true;
    long cyclesToRun = this->cpu->cycles + workUnitCycles;
    while(simBeginTicking && (this->cpu->cycles < cyclesToRun)){
        avrInstruction(this->cpu);
        this->cpu->tick();
        cyclesToRun = this->cpu->cycles + workUnitCycles;
    }
}
