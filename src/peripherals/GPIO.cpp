#include "GPIO.h"
#include <iostream>

AVRIOPort::AVRIOPort(CPU *cpu, AVRPortConfig *porConfig){

    // Add this to gpioPorts
    // cpu.gpioPorts.add(this);


}


void AVRIOPort::addListener(GPIOListener listner){
    this->listeners.push_back(listner);
}

void AVRIOPort::removeListener(GPIOListener listener){
    for(int i = 0; i < listeners.size(); i++){
        if(listeners[i] == listener){
            listeners.erase(listeners.begin() + i);
        }
    }
}

// PinState AVRIOPort::pinState(int index){
// }


void AVRIOPort::toggleInterrupt(u8 pin, bool risingEdge){

}

/*** TESTING ZONE ***/

// Create a sample GPIO listener
// void GListenTest(u8 value, u8 oldValue){
//     std::cout << "callin dat test func 1" << std::endl;
// }
// void GListenTest2(u8 value, u8 oldValue){
//     std::cout << "callin dat test func 2" << std::endl;
// }
// void GListenTest3(u8 value, u8 oldValue){
//     std::cout << "callin dat test func 3" << std::endl;
// }

int main(){

    /* UNOFFICIAL TEST 1: Add a GPIO listener*/
    // std::cout << "hi" << std::endl;
    // AVRIOPort *port = new AVRIOPort;
    // // Add it to the array using the addListener function
    // port->addListener(*GListenTest);
    // // Confirm it's in the array
    // std::cout << port->listeners.size() << std::endl;
    // if(port->listeners[0]){
    //     port->listeners[0](1, 1);
    // }

    /* UNOFFICIAL TEST 2: Remove GPIO listeners */
    // AVRIOPort *port = new AVRIOPort;
    // port->addListener(*GListenTest);
    // port->addListener(*GListenTest2);
    // port->addListener(*GListenTest3);
    // std::cout << "Size before removal: " << port->listeners.size() << std::endl;
    // for(int i = 0; i < port->listeners.size(); i++ ){
    //     port->listeners[i](1, 1);
    // }
    // port->removeListener(GListenTest3);
    // std::cout << "Size after removal 1: " << port->listeners.size() << std::endl;
    // for(int i = 0; i < port->listeners.size(); i++ ){
    //     port->listeners[i](1, 1);
    // }
    // port->removeListener(GListenTest2);
    // std::cout << "Size after removal 2: " << port->listeners.size() << std::endl;
    // for(int i = 0; i < port->listeners.size(); i++ ){
    //     port->listeners[i](1, 1);
    // }
    // port->removeListener(GListenTest);
    // std::cout << "Size after removal 3: " << port->listeners.size() << std::endl;
    // for(int i = 0; i < port->listeners.size(); i++ ){
    //     port->listeners[i](1, 1);
    // }
    // port->removeListener(GListenTest);
    // std::cout << "Size after removal 3: " << port->listeners.size() << std::endl;
    // for(int i = 0; i < port->listeners.size(); i++ ){
    //     port->listeners[i](1, 1);
    // }

}