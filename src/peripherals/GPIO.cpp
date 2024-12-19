#include "GPIO.h"
#include <iostream>
#include <functional>


/* Write Hook Function for DDR */
bool DDRWriteHook(u8 value, u8 oldValue, u16 address, u8 mask, CPU *cpu, AVRPortConfig *pConfig){
    // Read current port value
    u8 portValue = cpu->data[pConfig->PORT];
    // Update the DDR value
    cpu->data[pConfig->DDR] = value;

    // ***UNCOMMENT LATER***
    // writeGPIO(portValue, value);
    // updatePinRegister(value);
    return true;
}

/* Write Hook Function for output data register aka PORT */
bool PORTWriteHook(u8 value, u8 oldValue, u16 address, u8 mask, CPU *cpu, AVRPortConfig *pConfig){
    // Read current DDR status to determine which pins are input (0) or output (1)
    u8 DDRMask = cpu->data[pConfig->DDR];
    // update port with new values
    cpu->data[pConfig->PORT] = value;

    // ***UNCOMMENT LATER***
    // writeGPIO(value, DDRMask);
    // updatePinRegister(DDRMask);
    return true;

}

/* Write Hook Function for the input data register aka PIN — Writing 1 to PIN toggles PORT bits*/
bool PINWriteHook(u8 value, u8 oldValue, u16 address, u8 mask, CPU *cpu, AVRPortConfig *pConfig){
    // Read current port value
    u8 oldPortValue = cpu->data[pConfig->PORT];
    // Reads current DDR value
    u8 DDRMask = cpu->data[pConfig->DDR];
    // Now we toggle the port bits using XOR (ony 0+1 or 1+0 goes to 1)
    u8 portValue = oldPortValue ^ (value & mask);
    // And then update the PORT register
    cpu->data[pConfig->PORT] = portValue;

    // ***UNCOMMENT LATER***
    // writeGPIO(portValue, DDRMask);
    // updatePinRegister(DDRMask);
    return true;
}

/* Write Hook Function for PCIFR */
bool PCIFRWriteHook(u8 value, u8 oldValue, u16 address, u8 mask, CPU *cpu, AVRPortConfig *pConfig){

    //   cpu.writeHooks[PCIFR] = (value) => {
    //     for (const gpio of this.cpu.gpioPorts) {
    //       const { PCINT } = gpio;
    //       if (PCINT) {
    //         cpu.clearInterruptByFlag(PCINT, value);
    //       }
    //     }
    //     return true;
    //   };

    for(int i = 0; i < cpu->GPIOPorts.size(); i++){
        if(cpu->GPIOPorts[i]->PCINT){
            cpu->clearInterruptByFlag(cpu->GPIOPorts[i]->PCINT, value);
        }
    }
    return true;
}



/* Write Hook Function for PCMSK */
bool PCMSKWriteHook(u8 value, u8 oldValue, u16 address, u8 mask, CPU *cpu, AVRPortConfig *pConfig){

    //   cpu.writeHooks[PCMSK] = (value) => {
    //     cpu.data[PCMSK] = value;
    //     for (const gpio of this.cpu.gpioPorts) {
    //       const { PCINT } = gpio;
    //       if (PCINT) {
    //         cpu.updateInterruptEnable(PCINT, value);
    //       }
    //     }
    //     return true;
    //   };

    for(int i = 0; i < cpu->GPIOPorts.size(); i++){
        if(cpu->GPIOPorts[i]->PCINT){
            cpu->updateInterruptsEnabled(cpu->GPIOPorts[i]->PCINT, value);
        }
    }

    return true;
}


AVRIOPort::AVRIOPort(CPU *cpu, AVRPortConfig *portConfig){

    // Add this to gpioPorts
    // Note that push_back adds to the back of the vector (unlike the "add" for sets in TS)
    cpu->GPIOPorts.push_back(this);

    // Storing this AVRIOPort instance in the proper place in the GPIOByPort array
    cpu->GPIOByPort[portConfig->PORT] = this;

    /* And now, we set up some write hooks in the cpu for DDR, PORT, and PIN */
    cpu->writeHookFunctions[portConfig->DDR] = *DDRWriteHook;
    cpu->writeHookFunctions[portConfig->PORT] = *PORTWriteHook;
    cpu->writeHookFunctions[portConfig->PIN] = *PINWriteHook;

    /* Get set up for external interrupts */

    // We'll be using portConfig->externalInterrupts

    // Now, we need to map through the external interrupts...We are converting externalInterrupts (of type AVRExternalInterrupt) to externalInts (of type AVRInterruptConfig)

    /* Tested: ✅ */
    for(int i = 0; i < portConfig->externalInterrupts.size(); i++){
        if (portConfig->externalInterrupts[i]){

            AVRInterruptConfig *externalConfig = new AVRInterruptConfig;
            // address: externalConfig.interrupt,
            externalConfig->address = portConfig->externalInterrupts[i]->interrupt;
            // flagRegister: externalConfig.EIFR,
            externalConfig->flagRegister = portConfig->externalInterrupts[i]->EIFR;
            //  flagMask: 1 << externalConfig.index,
            externalConfig->flagMask = 1 << portConfig->externalInterrupts[i]->index;
            //  enableRegister: externalConfig.EIMSK,
            externalConfig->enableRegister = portConfig->externalInterrupts[i]->EIMSK;
            // enableMask: 1 << externalConfig.index,
            externalConfig->enableRegister = 1 << portConfig->externalInterrupts[i]->index;

            this->externalInts.push_back(externalConfig);
        }
        else {
            this->externalInts.push_back(nullptr);
        }
    }

    /* Circle back after we create the attach interrupts function */

    // const EICR = new Set(externalInterrupts.map((item) => item?.EICR));
    // for (const EICRx of EICR) {
    //   this.attachInterruptHook(EICRx || 0);
    // }
    // const EIMSK = externalInterrupts.find((item) => item && item.EIMSK)?.EIMSK ?? 0;
    // this.attachInterruptHook(EIMSK, 'mask');
    // const EIFR = externalInterrupts.find((item) => item && item.EIFR)?.EIFR ?? 0;
    // this.attachInterruptHook(EIFR, 'flag');

    /* And finally, we need to manage the pin change interrupts - responsible for detecting new rising or falling edges */

    // We will be using portConfig->pinChange

    // Configuring PCINT (of type AVRInterruptConfig) with the following
    if(portConfig->pinChange){
        // Initializing PCINT
        this->PCINT = new AVRInterruptConfig;

        // address: pinChange.pinChangeInterrupt,
        this->PCINT->address = portConfig->pinChange->pinChangeInterrupt;
        // flagRegister: pinChange.PCIFR,
        this->PCINT->flagRegister = portConfig->pinChange->PCIFR;
        // flagMask: 1 << pinChange.PCIE,
        this->PCINT->flagMask = 1 << portConfig->pinChange->PCIE;
        // enableRegister: pinChange.PCICR,
        this->PCINT->enableRegister = portConfig->pinChange->PCICR;
        // enableMask: 1 << pinChange.PCIE,
        this->PCINT->enableMask = 1 << portConfig->pinChange->PCIE;
    }
    else {
        this->PCINT = nullptr;
    }

    /* If pinChange is defined, the code attaches write hooks to the PCIFR (Pin Change Interrupt Flag Register) and PCMSK (Pin Change Mask Register). */
    if (portConfig->pinChange) {
        cpu->writeHookFunctions[portConfig->pinChange->PCIFR] = *PCIFRWriteHook;
        cpu->writeHookFunctions[portConfig->pinChange->PCMSK] = *PCMSKWriteHook;
    }
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

    /* UNOFFICIAL TEST: Make sure interrupts copy over from externalInterrupts to externalInts */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // std::cout << "here" << std::endl;
    // portDConfig *PD = new portDConfig;
    // std::cout << "here 2" << std::endl;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // std::cout << "here 3" << std::endl;
    // for(int i = 0; i < 4; i++){
    //     if(PD->externalInterrupts[i]){
    //     std::cout << "Port D externalInterrupts: " << PD->externalInterrupts[i] << std::endl;
    //     } 
    //     else {
    //     std::cout << "Port D externalInterrupts: nullptr" << std::endl;
    //     }
    //     if(testIO->externalInts[i]){
    //     std::cout << "Port D externalInts address should be 0, 0, 2, 4: " << int(testIO->externalInts[i]->address) << std::endl;
    //     }
    //     else {
    //     std::cout << "Port D externalInts address should be 0, 0, 2, 4: 0" << std::endl;            
    //     }
    // }

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

    /* Quick Write Hook Test */

    // // set up a test cpu const cpu = new CPU(new Uint16Array(1024), 8192);
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // Test portConfig
    /*
        portAConfig() {
        this->PIN = 0x20;
        this->DDR = 0x21;
        this->PORT = 0x22;
        // NO External interrupts
        this->externalInterrupts.clear(); 
    }
    */
    // AVRPortConfig *testConfig = new portAConfig;
    // AVRIOPort *ioTest = new AVRIOPort(cpu, testConfig);
    // cpu->writeData(testConfig->DDR, 10, 255, cpu, testConfig);
    // std::cout << "data at DDR: " << int(cpu->data[testConfig->DDR]) << std::endl;


}