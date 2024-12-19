#include "glueCode.h"
#include <iostream>

void AVRBoard::execute () {
    if (simActive) {
        // std::cout << "Executing..." << std::endl;
        // checks if needToQueueInterrupt = true
        if (needToQueueInterrupt) {
            // If true, queues up a sample interrupt with address of 5, NOT constant, and queue
            AVRInterruptConfig *testInterrupt = new AVRInterruptConfig;
            testInterrupt->address = 5;
            testInterrupt->constant = false;
            this->cpu->queueInterrupt(testInterrupt);

            // and enable interrupts by setting the flag in SREG
            this->cpu->data[95] = 0x80;

            // no Longer need to queue
            needToQueueInterrupt = false;
        }

        // Calls tick();
        this->cpu->tick();
    }
    else {
        // std::cout<< "Stoppin the sim" << std::endl;
        return;
    }
}

// For testing
// int main(int argc, char* argv[]) {

//     // Int to store which button has been presset
//     int userInput;

//     AVRBoard *board = new AVRBoard;

//     // Mimicing Unreal's "buttons"
//     while(1) {
//         std::cin >> userInput;

//         // 1 = start sim
//         if (userInput == 1) {
//             std::cout << "Simulation commence!" << std::endl;
//             board->simActive = true;
//         }
//         // 2 = queue interrupt
//         else if (userInput == 2) {
//             std::cout << "The time hath come to queue an interrupt!" << std::endl;
//             board->needToQueueInterrupt = true;
//         }
//         // 3 = stop sim
//         else if (userInput == 3) {
//             board->simActive = false;
//         }

//         board->execute();
//     }
// }