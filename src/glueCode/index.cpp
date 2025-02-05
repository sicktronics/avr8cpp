#include "index.h"

using namespace std;


// Set up the global vars LEDs
bool led13 = false;
bool oldLEDState = false;
bool led12 = false;
std::string hexiCode = "";
AVRRunner *runner = nullptr;

// int main(int argc, char* argv[]){

//     // If user input is 1, run compileAndRun();
//     // If user input is 0, terminate program.
//     cout << "Hello! Enter 1 to test the simulation. Enter zero to exit. >> ";
//     int userInput;
//     cin >> userInput;
//     cout << endl;
//     // 1 = start sim
//     if (userInput == 1) {
//         cout << "Simulation commence!" << endl;

//         compileAndRun();
//     }
//     // 0 = donezo
//     else{
//         std::cout << "Terminating program..." << std::endl;
//     }
// }


void executeProgram(std::string lilHexGal) {

    // Need to create a new AVR Runner from execute
    runner = new AVRRunner(lilHexGal);

    /*** Set up listeners to different ports below! ***/

    // Add listener to portB
   runner->portB->addListener(std::make_shared<std::function<void(u8, u8)>>(
    [](u8 value, u8 oldValue) {
        led13 = (runner->portB->pinState(5) == PinState::High);
        std::cout<< "THIS IS THE LED: " << led13 << std::endl;
    }));

    runner->execute();
}

void compileAndRun(){
    led13 = false;

    // Store user snippet

    // Ultimately, the try-catch statement that checks for either the errors or the hex file.
    // For now, the fake "build" that just returns the hex as a string
    hexiCode = buildHex();
    // std::cout << hexiCode << std::endl;

    // Since we are assuming that the result is sound hex, now we just call execute (will update later)
    std::cout << "Executing program..." << std::endl;
    executeProgram(hexiCode);

}