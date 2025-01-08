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

    /* Just for testing VVV */
    // Create a window
    // sf::Vector2f v1(350.f, 250.f);
    // sf::Vector2u v2(800, 600);
    // sf::VideoMode VidMode(v2);
    // sf::RenderWindow window(VidMode, "LED Blink Test");
    // Create a circle
    // sf::CircleShape circle(100.f); // Radius of 100
    // circle.setPosition(v1);  // Center the circle
    // circle.setFillColor(sf::Color::Black); // Initial color

    int variable = 0; // Variable to control the color

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

    // Add listener to portB
   runner->portB->addListener(std::make_shared<std::function<void(u8, u8)>>(
    [runner](u8 value, u8 oldValue) {
        led13 = (runner->portB->pinState(5) == PinState::High);
        std::cout << "THIS IS THE LED: " << led13 << std::endl;
        // appendBoolToFile("output.txt", led13);
        led12 = (runner->portB->pinState(4) == PinState::High);
        // std::cout << "THIS IS THE LED 12: " << led12 << std::endl;

        /* just for testing VV */

        // std::cout << "-----> CYCLE COUNT: " << runner->cpu->cycles << std::endl;
        // Waiting for 1/10 of a second
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));


        // sleep(1);
    }));

    std::cout << "--> ADDED LISTENER SUCCESSFULLY <--" << std::endl;

    // runner->execute(runner->cpu);
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

void appendBoolToFile(const std::string& filePath, bool value) {
    std::ofstream outFile(filePath, std::ios::app); // Open in append mode
    if (outFile.is_open()) {
        outFile << (value ? "true" : "false") << "\n"; // Append value and a newline
        outFile.close();
    } else {
        std::cerr << "Error: Unable to open file for appending." << std::endl;
    }
}