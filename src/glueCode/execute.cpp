#include "execute.h"
#include "intelHex.h"
#include <unistd.h>
#include <chrono>
#include <thread>
#include <iostream>

AVRRunner::AVRRunner(std::string lilHexGal){

    // First things first, we need to call loadHex() to load the hex into program memory
    std::vector<u16> program = std::vector<u16>(0);
    program.reserve(FLASH);
    // program.reserve(FLASH);
    std::vector<u8> programB = std::vector<u8>(0);
    // std::cout << programB.size() << std::endl;
    loadHex(lilHexGal, programB);
    // for (int i = 0; i < 100; i++){
    //     std::cout << int(programB[i]) << std::endl;
    // }

    // Now, we need to fill the u16 array with the u8 array contents
    for (size_t i = 0; i < programB.size(); i += 2) {
        if (programB.size() % 2 != 0) {
            throw std::runtime_error("programB size is not even.");
        }
        u16 value = programB[i] | programB[i + 1] << 8;
        // std::cout << value << std::endl;
        this->program.push_back(value);
    }
    std::cout << "--> CPU CONSTRUCTOR <--" << std::endl;
    std::cout << "B4 CONSTRUCTOR PROGRAM SIZE: " << this->program.size() << std::endl;
    cpu = new CPU(this->program);
    std::cout << "--> TIMER 0 CONSTRUCTOR <--" << std::endl;
    this->timer0 = new AVRTimer(this->cpu, new timer0Config);
    std::cout << "--> TIMER 1 CONSTRUCTOR <--" << std::endl;
    // this->timer1 = new AVRTimer(this->cpu, new timer1Config);
    std::cout << "--> TIMER 2 CONSTRUCTOR <--" << std::endl;
    // this->timer2 = new AVRTimer(this->cpu, new timer2Config);
    std::cout << "--> PORT B CONSTRUCTOR <--" << std::endl;
    this->portB = new AVRIOPort(this->cpu, new portBConfig);
    std::cout << "--> PORT C CONSTRUCTOR <--" << std::endl;
    this->portC = new AVRIOPort(this->cpu, new portCConfig);
    std::cout << "--> PORT D CONSTRUCTOR <--" << std::endl;
    this->portD = new AVRIOPort(this->cpu, new portDConfig);

    // Usart
    // Skip task scheduler
}

void AVRRunner::execute(){
    long cyclesToRun = this->cpu->cycles + workUnitCycles;
    while (this->cpu->cycles < cyclesToRun) {

        // std::cout << "-----> CYCLE COUNT: " << this->cpu->cycles << std::endl;
        // // Waiting for 1/10 of a second
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // std::cout << "--> BOUTTA EXEC <--" << std::endl;
        avrInstruction(this->cpu);
        // std::cout << "--> EXEC <--" << std::endl;
        this->cpu->tick();
        // std::cout << "--> TICK <--" << std::endl;
        

        cyclesToRun = this->cpu->cycles + workUnitCycles;
    }
    // std::cout << "REACHING END OF PROGRAM" << std::endl;
}
