#include "index.h"
#include "execute.h"
#include <fstream>
#include <string>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <iostream>
/* JUST FOR TESTING VVV */
// #include <SFML/Graphics.hpp>
// #include <iostream>

using namespace std;

/* Just for testing VVV */
// Function to get color based on a variable
// sf::Color getColor(int variable) {
//     switch (variable) {
//         case 0: return sf::Color::Red;
//         case 1: return sf::Color::Green;
//         case 2: return sf::Color::Black;
//         default: return sf::Color::White;
//     }
// }
/* * * * * * * * * * * * * * * * * * * * * * * * * */

// Set up the LEDs
bool led13 = false;
bool led12 = false;
std::string hexiCode = "";

int main(int argc, char* argv[]){

    // If user input is 1, run compileAndRun();
    // If user input is 0, terminate program.
    cout << "Hello! Enter 1 to test the simulation. Enter zero to exit. >> ";
    int userInput;
    cin >> userInput;
    cout << endl;
    // 1 = start sim
    if (userInput == 1) {
        cout << "Simulation commence!" << endl;

        compileAndRun();
    }
    // 0 = donezo
    else{
        std::cout << "Terminating program..." << std::endl;
    }
}

void executeProgram(std::string lilHexGal) {

    // Need to create a new AVR Runner from execute
    AVRRunner *runner = new AVRRunner(lilHexGal);

    std::cout << "--> CONSTRUCTION SUCCESSFUL <--" << std::endl;

    /*** Set up listeners to different ports below! ***/

    // Add listener to portB
   runner->portB->addListener(std::make_shared<std::function<void(u8, u8)>>(
    [runner](u8 value, u8 oldValue) {
        led13 = (runner->portB->pinState(5) == PinState::High);
        std::cout << "THIS IS THE LED: " << led13 << std::endl;
        led12 = (runner->portB->pinState(4) == PinState::High);
    }));

    std::cout << "--> ADDED LISTENER SUCCESSFULLY <--" << std::endl;

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