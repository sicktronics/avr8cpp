#include "GPIO.h"


/* Tested: ✅ */
AVRIOPort::AVRIOPort(CPU *cpu, AVRPortConfig *portConfig){

    // The CPU that gets passed in will be the "main cpu" referenced in our functions
    this->mainCPU = cpu;
    this->mainPortConfig = portConfig;
    // Ininitialize vectors
    this->externalClockListeners.resize(8);
    this->listeners.clear();

    // Add this to gpioPorts
    // Note that push_back adds to the back of the vector (unlike the "add" for sets in TS)
    cpu->GPIOPorts.push_back(this);

    // Storing this AVRIOPort instance in the proper place in the GPIOByPort array
    cpu->GPIOByPort[portConfig->PORT] = this;

    /* And now, we set up some write hooks in the cpu for DDR, PORT, and PIN */
    auto DDRWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
            [cpu, portConfig, this](u8 value, u8 oldValue, u16 address, u8 mask) 
            {
                std::cout << "Using DDR Write hook" << std::endl;
                // Read current port value
                u8 portValue = cpu->data[portConfig->PORT];
                // Update the DDR value
                cpu->data[portConfig->DDR] = value;

                // Set up with the port value and update the pin register
                this->writeGPIO(portValue, value);
                this->updatePinRegister(value);
                return true;
            });
    cpu->writeHookVector[portConfig->DDR] = DDRWriteHook;

    auto PORTWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [cpu, portConfig, this](u8 value, u8 oldValue, u16 address, u8 mask) 
        {
            std::cout << "Using PORT Write hook" << std::endl;
            // Read current DDR status to determine which pins are input (0) or output (1)
            u8 DDRMask = cpu->data[portConfig->DDR];
            // update port with new values
            cpu->data[portConfig->PORT] = value;

            // ***UNCOMMENT LATER***
            this->writeGPIO(value, DDRMask);
            this->updatePinRegister(DDRMask);
            return true;
        });
    cpu->writeHookVector[portConfig->PORT] = PORTWriteHook;
    
    /* Write Hook Function for the input data register aka PIN — Writing 1 to PIN toggles PORT bits*/
    auto PINWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [cpu, portConfig, this](u8 value, u8 oldValue, u16 address, u8 mask) 
        {
            std::cout << "Using PIN Write hook" << std::endl;
            // Read current port value
            u8 oldPortValue = cpu->data[portConfig->PORT];
            // Reads current DDR value
            u8 DDRMask = cpu->data[portConfig->DDR];
            // Now we toggle the port bits using XOR (ony 0+1 or 1+0 goes to 1)
            u8 portValue = oldPortValue ^ (value & mask);
            // And then update the PORT register
            cpu->data[portConfig->PORT] = portValue;

            // ***UNCOMMENT LATER***
            this->writeGPIO(portValue, DDRMask);
            this->updatePinRegister(DDRMask);
            return true;
        });
    cpu->writeHookVector[portConfig->PIN] = PINWriteHook;

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

    // Extract unique EICR values from the external interrupts
    std::set<u8> EICR;
    for (const auto &interrupt : this->mainPortConfig->externalInterrupts) {
        if (interrupt) {
            EICR.insert(interrupt->EICR);
        }
    }

    // Attach interrupt hooks for each unique EICR value
    for (const auto &EICRx : EICR) {
        this->attachInterruptHook(EICRx);
    }

    // Find the EIMSK register in the external interrupts and attach a mask interrupt hook
    u8 EIMSK = 0;
    for (const auto &interrupt : this->mainPortConfig->externalInterrupts) {
        if (interrupt && interrupt->EIMSK) {
            EIMSK = interrupt->EIMSK;
            break;
        }
    }
    this->attachInterruptHook(EIMSK, "mask");

    // Find the EIFR register in the external interrupts and attach a flag interrupt hook
    u8 EIFR = 0;
    for (const auto &interrupt : this->mainPortConfig->externalInterrupts) {
        if (interrupt && interrupt->EIFR) {
            EIFR = interrupt->EIFR;
            break;
        }
    }
    this->attachInterruptHook(EIFR, "flag");

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
        // Write hook for PCIFR
        auto PCIFRWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [cpu, portConfig](u8 value, u8 oldValue, u16 address, u8 mask) 
        {       
                std::cout << "Using PCIFR Write hook" << std::endl;    
                // Loop over GPIO ports and if one has a valid PCINT, clear the interrupt
                for(int i = 0; i < cpu->GPIOPorts.size(); i++){
                    if(cpu->GPIOPorts[i]->PCINT){
                        cpu->clearInterruptByFlag(cpu->GPIOPorts[i]->PCINT, value);
                    }
                }
                return true;
        });
        cpu->writeHookVector[portConfig->pinChange->PCIFR] = PCIFRWriteHook;
        // Write hook for PCMSK
        auto PCMSKWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [cpu, portConfig](u8 value, u8 oldValue, u16 address, u8 mask) 
        {
            // Update interrupts enabled for each port with a valid PCINT
            std::cout << "Using PCMSK Write hook" << std::endl;
            for(int i = 0; i < cpu->GPIOPorts.size(); i++){
                if(cpu->GPIOPorts[i]->PCINT){
                    cpu->updateInterruptsEnabled(cpu->GPIOPorts[i]->PCINT, value);
                }
            }

            return true;
        });
        cpu->writeHookVector[portConfig->pinChange->PCMSK] = PCMSKWriteHook;
    }
}

/* Tested: ✅ */
void AVRIOPort::addListener(GPIOListener listner){
    this->listeners.push_back(listner);
}

/* Tested: ✅ */
void AVRIOPort::removeListener(GPIOListener listener){
    for(int i = 0; i < listeners.size(); i++){
        if(listeners[i] == listener){
            listeners.erase(listeners.begin() + i);
        }
    }
}

/* Tested: ✅ */
PinState AVRIOPort::pinState(u8 index){

    // Retrieve DDR and PORT register values
    const u8 ddr = this->mainCPU->data[this->mainPortConfig->DDR];
    const u8 port = this->mainCPU->data[this->mainPortConfig->PORT];

    // Calculate the bit mask for the specified pin
    const u8 bitMask = 1 << index;

    // Determine the open state (Input or InputPullUp)
    PinState openState = (port & bitMask) ? PinState::InputPullUp : PinState::Input;

    // Determine the high value state based on openCollector
    PinState highValue = (this->openCollector & bitMask) ? openState : PinState::High;

    // Return the appropriate state based on DDR and lastValue
    if (ddr & bitMask) {
        return (this->lastValue & bitMask) ? highValue : PinState::Low;
    } else {
        return openState;
    }
}

/* Tested: ✅ */
void AVRIOPort::setPin(u8 index, bool value) {
    // Calculate the bit mask for the specified pin
    u8 bitMask = 1 << index;

    // Clear the bit for this pin in pinValue
    this->pinValue &= ~bitMask;

    // Set the bit if the value is true
    if (value) {
        this->pinValue |= bitMask;
    }

    // Update the PIN register using the current DDR value
    this->updatePinRegister(this->mainCPU->data[this->mainPortConfig->DDR]);
}

/* Tested: ✅ */
void AVRIOPort::timerOverridePin(u8 pin, PinOverrideMode mode){

    // Grabbin some quick reffies to the main portConfig and cpu 
    const AVRPortConfig *portConfig = this->mainPortConfig;
    CPU *cpu = this->mainCPU;
    // Getting a lil pinMask filter set up
    u8 pinMask = 1 << pin;

    if (mode == PinOverrideMode::None) {
        // Remove override for this pin
        this->overrideMask |= pinMask;
        this->overrideValue &= ~pinMask;
        std::cout << "Mode none" << std::endl;
    } else {
        // Apply override for this pin
        this->overrideMask &= ~pinMask;

        switch (mode) {
            case PinOverrideMode::Enable:
                this->overrideValue &= ~pinMask;
                this->overrideValue |= cpu->data[portConfig->PORT] & pinMask;
                std::cout << "Mode enable" << std::endl;
                break;

            case PinOverrideMode::Set:
                this->overrideValue |= pinMask;
                std::cout << "Mode set" << std::endl;
                break;

            case PinOverrideMode::Clear:
                this->overrideValue &= ~pinMask;
                std::cout << "Mode clear" << std::endl;
                break;

            case PinOverrideMode::Toggle:
                this->overrideValue ^= pinMask;
                std::cout << "Mode toggers" << std::endl;
                break;

            default:
                break;
        }
    }

    // Update the GPIO and PIN registers
    u8 ddrMask = cpu->data[portConfig->DDR];
    this->writeGPIO(cpu->data[portConfig->PORT], ddrMask);
    this->updatePinRegister(ddrMask);
}

/* Tested: ✅ */
void AVRIOPort::updatePinRegister(u8 ddr){
    // Calculate the new pin value
    u8 newPin = (this->pinValue & ~ddr) | (this->lastValue & ddr);

    // Update the CPU's PIN register
    this->mainCPU->data[this->mainPortConfig->PIN] = newPin;

    // Check if the PIN value has changed
    if (this->lastPin != newPin) {
        std::cout << "newPin " << int(newPin) << " vs last pin " << int(this->lastPin) << std::endl;
        for (u8 index = 0; index < 8; index++) {
            // Check if the value of this pin has changed
            if ((newPin & (1 << index)) != (this->lastPin & (1 << index))) {
                std::cout << "value of pin " << int(index) << " has changed" << std::endl;
                bool value = (newPin & (1 << index)) != 0;

                // Toggle the interrupt for this pin
                this->toggleInterrupt(index, value);

                // Notify the external clock listener if it exists
                if (this->externalClockListeners[index]) {
                    std::cout << "Checking the clock listener" << std::endl;
                    this->externalClockListeners[index](value);
                }
            }
        }

        // Update the lastPin value
        this->lastPin = newPin;
    }
}

/* Tested: ✅ */
void AVRIOPort::toggleInterrupt(u8 pin, bool risingEdge){
    // Pointer to the main port configuration
    const AVRPortConfig *portConfig = this->mainPortConfig;
    // Reference to the port config's external interrupts vector
    const std::vector<AVRExternalInterrupt *> &externalInterrupts = portConfig->externalInterrupts;
    // Pointer to the port config's pinChange
    const AVRPinChangeInterrupt *pinChange = portConfig->pinChange;
    // Pointer to the AVRIOPORT's external interrupt config at a given pin
    AVRInterruptConfig *external = this->externalInts[pin];
    // Pointer to the ~port config's~ external interrupt at a given pin
    const AVRExternalInterrupt *externalConfig = externalInterrupts[pin];
    std::cout<< "And here we are, togglin..." << std::endl;
    // Now we handle the external interrupt
    if (external && externalConfig) {
        std::cout<< "Check 0" << std::endl;
        const u8 EIMSK = externalConfig->EIMSK;
        const u8 index = externalConfig->index;
        const u8 EICR = externalConfig->EICR;
        const u8 iscOffset = externalConfig->ISCOffset;

        // If the data at EIMSK and 1 shifted to the index location is nonzero
        if (this->mainCPU->data[EIMSK] & (1 << index)) {
            std::cout<< "Check 1" << std::endl;
            // Set up the configuration
            const u8 configuration = (this->mainCPU->data[EICR] >> iscOffset) & 0x3;
            std::cout << "Configuration is: " << int(configuration) << std::endl; 
            bool generateInterrupt = false;
            external->constant = false;
            // Now we check the interrupt mode via the configuration (either 0, 1, 2, or 3)
            switch (static_cast<InterruptMode>(configuration)) {
                case InterruptMode::LowLevel:
                    generateInterrupt = !risingEdge;
                    external->constant = true;
                    break;
                case InterruptMode::Change:
                    generateInterrupt = true;
                    break;
                case InterruptMode::FallingEdge:
                    generateInterrupt = !risingEdge;
                    break;
                case InterruptMode::RisingEdge:
                    generateInterrupt = risingEdge;
                    break;
            } 
            // Then we check if we should generate an interrupt or clear it
            if (generateInterrupt) {
                this->mainCPU->setInterruptFlag(external);
            } else if (external->constant) {
                this->mainCPU->clearInterrupt(external, true);
            }       
        }
    }

    // Next, we check for and handle pin change interrupts - 1 & 1 & 1
    if (pinChange && this->PCINT && (pinChange->mask & (1 << pin))) {
        const u8 PCMSK = pinChange->PCMSK;
        std::cout << "Confirm Pin change working" << std::endl;
        // If the PCMSK register is in the correct state, set a new interrupt flag
        if (this->mainCPU->data[PCMSK] & (1 << (pin + pinChange->offset))) {
            std::cout << "Setting interrupt flag" << std::endl;
            this->mainCPU->setInterruptFlag(this->PCINT);
        }
    }
}

/* Tested: ✅ */
void AVRIOPort::attachInterruptHook(int reg, std::string registerType){
    // Return if register is invalid
    if(reg == 0){
        std::cout << "NUH UH" << std::endl;
        return;
    }
    // Capture CPU and other necessary state in a lambda function
    auto interruptWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this, reg, registerType](u8 value, u8 oldValue, u16 address, u8 mask) -> bool 
        {
        // Handle the register update if not a flag type
        if (registerType != "flag") {
            std::cout << "We got a NOT flag" << std::endl;
            this->mainCPU->data[reg] = value;
        }

        // Iterate through GPIO ports
        for (auto *gpio : this->mainCPU->GPIOPorts) {
            // Iterate through external interrupts for each GPIO
            for (auto *external : gpio->externalInts) {
                if (external) {
                    std::cout << "We got an EXTERNAL" << std::endl;
                    if (registerType == "mask") {
                        std::cout << "We got a MASK" << std::endl;
                        this->mainCPU->updateInterruptsEnabled(external, value);
                    } else if (!external->constant && registerType == "flag") {
                        std::cout << "We got an EXTERNAL FLAG" << std::endl;
                        this->mainCPU->clearInterruptByFlag(external, value);
                    }
                } else {
                    std::cout << "NO EXTERNAL" << std::endl;
                }
                
            }

            // Check for any external interrupts on the GPIO
            gpio->checkExternalInterrupts();
        }

        // Indicate the hook handled the write
        return true;
    });
    mainCPU->writeHookVector[reg] = interruptWriteHook;
    std::cout << "Hook added successfully" << std::endl;
}

// Tested: correctly queues or skips basend on the the contents of EIMSK, pinValue, etc. ✅
void AVRIOPort::checkExternalInterrupts(){

    // We'll be using mainPortConfig->externalInterrupts and mainCPU
    
    /* Iterate over all 8 pins of the GPIO port*/
    for(int pin = 0; pin < 8; pin ++) {
        // Skip pins that do not have an associated external interrupt
        AVRExternalInterrupt *external = mainPortConfig->externalInterrupts[pin];
        if(external == nullptr) {
            // std::cout << "check 1 true - nullptr" << std::endl;
            continue;
        }
        // Keep lastPin states that are 1 - tested! ✅
        const bool pinValue = (this->lastPin & (1 << pin)) != 0 ? true : false;

        // std::cout << "pinValue: " << pinValue << std::endl;

        // if(external->index == 0){
        //     // std::cout << "We have an int0" << std::endl;
        // }
        // if (external->index == 1) {
        //     // std::cout << "We have an int1" << std::endl;
        // }

        // If pinValue is HIGH (cuz we're triggering on low level) or if interrupts are NOT enabled, then skip!
        if((mainCPU->data[external->EIMSK] & (1 << external->index)) == 0 || pinValue) {
            // std::cout << int(mainCPU->data[external->EIMSK]) << std::endl;
            // std::cout << "check 2 true " << std::endl;
            continue;
        }
        // Read the control register, bit shift to align this pin's interrupt configuration, and & with 0b11 to extract the 2-bit configuration value for this pin.
        const u8 configuration = (mainCPU->data[external->EICR] >> external->ISCOffset) & 0x3;
        if(configuration == int(InterruptMode::LowLevel)) {
            AVRInterruptConfig *newInt = new AVRInterruptConfig;
            newInt->address = external->interrupt;
            newInt->flagRegister = external->EIFR;
            newInt->flagMask = 1 << external->index;
            newInt->enableRegister = external->EIMSK;
            newInt->enableMask = 1 << external->index;
            newInt->constant = true;
            mainCPU->queueInterrupt(newInt);
            // std::cout << "queued a new interrupt!" << std::endl;
        }
    }
}

/* Tested: ✅ */
void AVRIOPort::writeGPIO(u8 value, u8 ddr){

    // Calculate the new GPIO value based on the value, DDR, and override settings
    u8 newValue = (((value & this->overrideMask) | this->overrideValue) & ddr) | (value & ~ddr);
    std::cout << "writeGPIO's calculated newValue: " << int(newValue) << std::endl;
    u8 prevValue = this->lastValue;

    // Check if the new value or DDR has changed
    if (newValue != prevValue || ddr != this->lastDdr) {
        // Update the last stored values
        this->lastValue = newValue;
        this->lastDdr = ddr;

        // Notify all GPIO listeners of the change
        for (const auto &listener : this->listeners) {
            std::cout << "gonna invoke the listener" << std::endl;
            listener(newValue, prevValue);
        }
    }
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

// int main(){


    /* testing constructor ✅*/
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // cpu->writeData(testIO->mainPortConfig->DDR, 1);
    // cpu->writeData(testIO->mainPortConfig->PORT, 2);
    // cpu->writeData(testIO->mainPortConfig->PIN, 3);
    // cpu->writeData(testIO->mainPortConfig->pinChange->PCIFR, 4);
    // cpu->writeData(testIO->mainPortConfig->pinChange->PCMSK, 5);
    // // The three interrupt addresses...
    // cpu->writeData(0x69, 6);
    // cpu->writeData(0x3d, 7);
    // cpu->writeData(0x3c, 8);


    /* testing pinState() ✅*/
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // DDR is INPUT
    // cpu->data[PD->DDR] = 0b00000000;
    // is OUTPUT
    // cpu->data[PD->DDR] = 0b11111111;
    // testIO->lastValue = 0b11111111;
    // cpu->data[PD->PORT] = 0b00000000;
    // u8 index = 2;
    // std::cout << int(testIO->pinState(2)) << std::endl;

    /* testing setPin() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // testIO->pinValue = 0b00000000;
    // cpu->data[PD->DDR] = 0b00000000;
    // // PIN before:
    // cpu->data[PD->PIN] = testIO->pinValue;
    // std::cout << "data at PIN before: " << int(cpu->data[PD->PIN]) << std::endl;
    // // Test with clearing a bit
    // // testIO->setPin(7, false);
    // // Test with setting a bit
    // testIO->setPin(0, true);
    // // Check to confirm that location in memory is changed
    // std::cout << "data at PIN after: " << int(cpu->data[PD->PIN]) << std::endl;

    /* testing timerOverridePin() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // testIO->timerOverridePin(4, PinOverrideMode::None);


    /* testing updatePinRegister() ✅ */
    // void fakeClkList(bool pinValue){
    // std::cout << "Inside the listener! pinValue: " << pinValue << std::endl;
    // }
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // testIO->pinValue = 0b11111111;
    // testIO->lastValue = 0b11110000;
    // testIO->externalClockListeners[4] = *fakeClkList;
    // std::cout << testIO->externalClockListeners.size() << std::endl;
    // testIO->updatePinRegister(0b10101010);
    // std::cout << "Register should be updated with new pin: " << int(testIO->mainCPU->data[testIO->mainPortConfig->PIN]) << std::endl;   

 /* testing writeGPIO() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // testIO->addListener(*fakeLis);
    // std::cout << testIO->listeners.size() << std::endl;
    // // ddr should be all 1s and value should be nonzero
    // testIO->writeGPIO(20, 0);
    // Fake listener
    // void fakeLis(u8 value, u8 oldValue){
    //     std::cout << "old value: " << int(oldValue) << std::endl;
    //     std::cout << "new value: " << int(value) << std::endl;
    // }


    /* testing toggleInterrupt() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // Testing external interrupts ✅
    // Hard-writing data to EIMSK of INT0 and INT1
    // cpu->writeData(0x3d, 255);
    // cpu->writeData(0x69, 255);
    // cpu->data[0x6d] = 255;
    // If pin = 2, externalInterrupts[2] should be INT0 and externalInts[2] has corresponding interruptconfig
    // So for INT0:
    /*
        this->EICR = 0x69;
        this->EIMSK = 0x3d;
        this->EIFR = 0x3c;
        this->index = 0;
        this->ISCOffset = 0;
        this->interrupt = 2;
    */
    // Should move into Check 0, and Check 1
    //    testIO->toggleInterrupt(3,true);
    // Testing pinChange interrupts for PCINT2 - conditionals act as expected ✅

    /* testing attachInterruptHook() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // int addr = 10;
    // int val = 8;
    // // attach
    // testIO->attachInterruptHook(addr, "mask");
    // cpu->writeData(addr, val);
    // std::cout << int(cpu->data[addr]) << std::endl;

    /* TESTING checkExternalInterrupts ✅*/
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);

    // portDConfig *PD = new portDConfig;

    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);

    // TEST 1: TESTING THE WRITE HOOKS IN THE CONSTRUCTOR ✅
    // cpu->writeData(0x2a, 12); // DDR address for PORTD
    // std::cout << int(cpu->data[0x2a]) << std::endl;
    // std::cout << int(cpu->data[0x2b]) << std::endl;
    // std::cout << int(cpu->data[0x29]) << std::endl;
    // cpu->writeData(0x2b, 12); // PORT address for PORTD
    // std::cout << int(cpu->data[0x2a]) << std::endl;
    // std::cout << int(cpu->data[0x2b]) << std::endl;
    // std::cout << int(cpu->data[0x29]) << std::endl;
    // cpu->writeData(0x29, 12); // PIN address for PORTD
    // std::cout << int(cpu->data[0x2a]) << std::endl;
    // std::cout << int(cpu->data[0x2b]) << std::endl;
    // std::cout << int(cpu->data[0x29]) << std::endl;






    // testIO->checkExternalInterrupts();

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


    /* */

    // u8 lastPin = 0b00001000;

    // bool pinsVal[8];

    // for(int pin = 0; pin < 8; pin ++) {

    //     const bool pinValue = (lastPin & (1 << pin)) != 0 ? true : false;
    //     pinsVal[pin] = pinValue;

    //     std::cout << pin << ": " << bool(pinsVal[pin]) << " " << std::endl;

    //     /*
    //     const pinValue = !!(this.lastPin & (1 << pin));
    //     const { EIFR, EIMSK, index, EICR, iscOffset, interrupt } = external;
    //     if (!(cpu.data[EIMSK] & (1 << index)) || pinValue) {
    //         continue;
    //     }
    //     */
    // }
    // std::cout << std::endl;

// }