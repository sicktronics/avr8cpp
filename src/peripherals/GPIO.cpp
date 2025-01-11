#include "GPIO.h"

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  CPP for the GPIO module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

AVRIOPort::AVRIOPort(CPU *cpu, AVRPortConfig *portConfig){

    // The CPU that gets passed in will be the "main cpu" referenced in our functions
    this->mainCPU = cpu;
    this->mainPortConfig = portConfig;
    // Set up the externalClockListeners with the correct size and fill with nullptr
    this->externalClockListeners = std::vector<std::shared_ptr<std::function<void(bool)>>>(8, nullptr);
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
    
    /* Write Hook Function for the PORT */
    auto PORTWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [cpu, portConfig, this](u8 value, u8 oldValue, u16 address, u8 mask) 
        {
            std::cout << "Using PORT Write hook" << std::endl;
            // Read current DDR status to determine which pins are input (0) or output (1)
            u8 DDRMask = cpu->data[portConfig->DDR];
            // update port with new values
            cpu->data[portConfig->PORT] = value;
            this->writeGPIO(value, DDRMask);
            this->updatePinRegister(DDRMask);
            return true;
        });
    cpu->writeHookVector[portConfig->PORT] = PORTWriteHook;
    
    /* Write Hook Function for the input data register aka PIN — Writing 1 to PIN toggles PORT bits*/
    auto PINWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [cpu, portConfig, this](u8 value, u8 oldValue, u16 address, u8 mask) 
        {
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

    // Now, we need to map through the external interrupts...We are converting externalInterrupts (of type AVRExternalInterrupt) to externalInts (of type AVRInterruptConfig)
    for(int i = 0; i < portConfig->externalInterrupts.size(); i++){
        if (portConfig->externalInterrupts[i]){

            AVRInterruptConfig *externalConfig = new AVRInterruptConfig;
            externalConfig->address = portConfig->externalInterrupts[i]->interrupt;
            externalConfig->flagRegister = portConfig->externalInterrupts[i]->EIFR;
            externalConfig->flagMask = 1 << portConfig->externalInterrupts[i]->index;
            externalConfig->enableRegister = portConfig->externalInterrupts[i]->EIMSK;
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

    // Configuring PCINT (of type AVRInterruptConfig) with the following
    if(portConfig->pinChange){
        // Initializing PCINT
        this->PCINT = new AVRInterruptConfig;
        this->PCINT->address = portConfig->pinChange->pinChangeInterrupt;
        this->PCINT->flagRegister = portConfig->pinChange->PCIFR;
        this->PCINT->flagMask = 1 << portConfig->pinChange->PCIE;
        this->PCINT->enableRegister = portConfig->pinChange->PCICR;
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
        // std::cout << "Mode none" << std::endl;
    } else {
        // Apply override for this pin
        this->overrideMask &= ~pinMask;
        switch (mode) {
            case PinOverrideMode::Enable:
                this->overrideValue &= ~pinMask;
                this->overrideValue |= cpu->data[portConfig->PORT] & pinMask;
                // std::cout << "Mode enable" << std::endl;
                break;

            case PinOverrideMode::Set:
                this->overrideValue |= pinMask;
                // std::cout << "Mode set" << std::endl;
                break;

            case PinOverrideMode::Clear:
                this->overrideValue &= ~pinMask;
                // std::cout << "Mode clear" << std::endl;
                break;

            case PinOverrideMode::Toggle:
                this->overrideValue ^= pinMask;
                // std::cout << "Mode toggers" << std::endl;
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

void AVRIOPort::updatePinRegister(u8 ddr){
    // Calculate the new pin value
    u8 newPin = (this->pinValue & ~ddr) | (this->lastValue & ddr);
    // Update the CPU's PIN register
    this->mainCPU->data[this->mainPortConfig->PIN] = newPin;
    // Check if the PIN value has changed
    if (this->lastPin != newPin) {
        for (u8 index = 0; index < 8; index++) {
            // Check if the value of this pin has changed
            if ((newPin & (1 << index)) != (this->lastPin & (1 << index))) {
                bool value = (newPin & (1 << index)) != 0;
                // Toggle the interrupt for this pin
                this->toggleInterrupt(index, value);
                // Notify the external clock listener if it exists
                if (this->externalClockListeners[index]) {
                    // std::cout << "Checking the clock listener" << std::endl;
                    (*this->externalClockListeners[index])(value);
                }
            }
        }
        // Update the lastPin value
        this->lastPin = newPin;
    }
}

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
    // Now we handle the external interrupt
    if (external && externalConfig) {
        const u8 EIMSK = externalConfig->EIMSK;
        const u8 index = externalConfig->index;
        const u8 EICR = externalConfig->EICR;
        const u8 iscOffset = externalConfig->ISCOffset;
        // If the data at EIMSK and 1 shifted to the index location is nonzero
        if (this->mainCPU->data[EIMSK] & (1 << index)) {
            // Set up the configuration
            const u8 configuration = (this->mainCPU->data[EICR] >> iscOffset) & 0x3;
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
        // If the PCMSK register is in the correct state, set a new interrupt flag
        if (this->mainCPU->data[PCMSK] & (1 << (pin + pinChange->offset))) {
            this->mainCPU->setInterruptFlag(this->PCINT);
        }
    }
}

void AVRIOPort::attachInterruptHook(int reg, std::string registerType){
    // Return if register is invalid
    if(reg == 0){
        return;
    }
    // Capture CPU and other necessary state in a lambda function
    auto interruptWriteHook = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this, reg, registerType](u8 value, u8 oldValue, u16 address, u8 mask) -> bool 
        {
        // Handle the register update if not a flag type
        if (registerType != "flag") {
            this->mainCPU->data[reg] = value;
        }

        // Iterate through GPIO ports
        for (auto *gpio : this->mainCPU->GPIOPorts) {
            // Iterate through external interrupts for each GPIO
            for (auto *external : gpio->externalInts) {
                if (external) {
                    if (registerType == "mask") {
                        this->mainCPU->updateInterruptsEnabled(external, value);
                    } else if (!external->constant && registerType == "flag") {
                        this->mainCPU->clearInterruptByFlag(external, value);
                    }
                } else {
                }
            }
            // Check for any external interrupts on the GPIO
            gpio->checkExternalInterrupts();
        }
        // Indicate the hook handled the write
        return true;
    });
    mainCPU->writeHookVector[reg] = interruptWriteHook;
}

void AVRIOPort::checkExternalInterrupts(){
    /* Iterate over all 8 pins of the GPIO port*/
    for(int pin = 0; pin < 8; pin ++) {
        // Skip pins that do not have an associated external interrupt
        AVRExternalInterrupt *external = mainPortConfig->externalInterrupts[pin];
        if(external == nullptr) {
            continue;
        }
        // Keep lastPin states that are 1 - tested! ✅
        const bool pinValue = (this->lastPin & (1 << pin)) != 0 ? true : false;
        // If pinValue is HIGH (cuz we're triggering on low level) or if interrupts are NOT enabled, then skip!
        if((mainCPU->data[external->EIMSK] & (1 << external->index)) == 0 || pinValue) {
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
        }
    }
}

void AVRIOPort::writeGPIO(u8 value, u8 ddr){
    // Calculate the new GPIO value based on the value, DDR, and override settings
    u8 newValue = (((value & this->overrideMask) | this->overrideValue) & ddr) | (value & ~ddr);
    u8 prevValue = this->lastValue;

    // Check if the new value or DDR has changed
    if (newValue != prevValue || ddr != this->lastDdr) {
        // Update the last stored values
        this->lastValue = newValue;
        this->lastDdr = ddr;

        // Notify all GPIO listeners of the change
        for (const auto &listener : this->listeners) {
            (*listener)(newValue, prevValue);
        }
    }
}