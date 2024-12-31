#include "CPU.h"
#include "interrupt.h"
#include <iostream>


CPU::CPU(std::vector<u16> progMem, int SRAMSize){

    // EXPLICITLY CLEARING TIMER 0's TIMSK LOCATION
    // this->data[110] = 0;
    // std::fill(data, arr + 5, 0);
    
    // Filling data[] with zeros bb!!
    std::fill(data, data + data_array_size, 0);

    this->SRAM_BYTES = SRAMSize;

    // Set up the writeHookVector and readHookFunctions with the correct size and fill with nullptr
    this->writeHookVector = std::vector<std::shared_ptr<std::function<bool(u8, u8, u16, u8)>>>(8448, nullptr);
    this->readHookFunctions = std::vector<readHookFunction>(8448, nullptr);

    // Next, to fill the vector with null?
    // std::cout << "PROG MEM SIZE:: " << progMem.size() << std::endl;

    // STEP 1: fill program memory with contents of progMem
    for(int i = 0; i < progMem.size(); i++) {
        this->programMemory.push_back(progMem[i]);
        // std::cout << "Program Memory at " << i << ": " << this->programMemory[i] << std::endl; 
    }

    /* ~TRBL TEST 1~ and ~TRBL TEST 1b~: Check the output of program memory: */
    // for(int i = 0; i < 100; i++) {
    //     std::cout << int(programMemory[i]) << std::endl;
    // }

    // std::cout << "THIS->PROGRAM MEMORY SIZE: " << this->programMemory.size() << std::endl;

    // STEP 2: Fill programBytes with the contents of programMemory but in 8-bit chunks
   // Resize programBytes
   programBytes.resize(programMemory.size() * 2);

   for (int i = 0; i < programMemory.size(); i++) {
        // Fill the array in LITTLE-ENDIAN style (least-significant byte at lower address)
        programBytes[2 * i] = progMem[i] & 0xFF;            // Lower 8 bits
        // std::cout << "progBytes at " << 2*i << ": " << int(programBytes[2 * i]) << std::endl;
        programBytes[2 * i + 1] = (progMem[i] >> 8) & 0xFF; // Upper 8 bits
        // std::cout << "progBytes at " << 2*i+1 << ": " << int(programBytes[2 * i + 1]) << std::endl;
   }

    // Updating 22 bit addressing based on size of programBytes
    this->pc22Bits = this->programBytes.size() > 0x20000 ? true : false;

    this->reset();
}

void CPU::reset(){

/**
   * reset
   * - points stack pointer to last data memory location
   * - resets program counter
   * - empty pending interrupts
   * - reset next interrupt
   * - reset nextclockevent
**/
    // Reset stack pointer
    setSP(SRAM_BYTES + REGISTER_SPACE - 1);
    
    // Reset program counter
    this->PC = 0;

    // Fill pending interrupts with nullptr - test
    std::fill(pendingInterrupts, (pendingInterrupts + MAX_INTERRUPTS), nullptr);
    // for (int i = 0; i < MAX_INTERRUPTS; i++){
    //     std::cout << pendingInterrupts[i] << std::endl;
    // }

    // Reset the next interrupt
    nextInterrupt = -1;

    // Reset the next clock event
    this->nextClockEvent = nullptr;
}

/*
Function for setting the position of the SP
*/
void CPU::setSP(u16 value) {
    setUint16LittleEndian(93, value);
}
/*
Function for getting the position of the SP
*/
u16 CPU::getSP() {
    return getUint16LittleEndian(93);
}
/*
Returns a value ASSUMING it's stored in little-endian format
*/
u16 CPU::getUint16LittleEndian(int byteOffset){
    
    // Get lower 8 bits first, shift them over, then get upper 8 bits
    u16 value = (this->data[byteOffset] & 0xFF) | ((this->data[byteOffset+1]) << 8);
    
    return value;
}

void CPU::setUint16LittleEndian(int byteOffset, u16 value){
    
    this->data[byteOffset] = value & 0xFF; // Lower 8 bits
    this->data[byteOffset+1] = (value >> 8) & 0xFF; // Upper 8 bits

//     for (int i = 0; i < programMemory.size(); i++) {
//         // Fill the array in LITTLE-ENDIAN style (least-significant byte at lower address)
//         programBytes[2 * i] = progMem[i] & 0xFF;            // Lower 8 bits
//         // std::cout << "progBytes at " << 2*i << ": " << int(programBytes[2 * i]) << std::endl;
//         programBytes[2 * i + 1] = (progMem[i] >> 8) & 0xFF; // Upper 8 bits
//         // std::cout << "progBytes at " << 2*i+1 << ": " << int(programBytes[2 * i + 1]) << std::endl;
//    }
}

void CPU::setInt16LittleEndian(u16 address, i16 value) {
    this->data[address] = value & 0xFF;         // Lower byte
    this->data[address + 1] = (value >> 8) & 0xFF; // Upper byte
}

// typedef bool (*writeHookFunction) (u8 value, u8 oldValue, u16 address, u8 mask);
void CPU::writeData(u16 address, u8 value, u8 mask){

    // Grabbing a pointer to a given function
    // writeHookFunction hookPtr = writeHookFunctions[address];
    auto hookPtr = this->writeHookVector[address];

    // If hookPtr exists
    if(hookPtr != nullptr) {
        // std::cout << "a hook exists at location " << address << std::endl;
        // If, by calling the writeHookFunction, we write the value at the location,
        // then we are already done!
        // !!!FOR NOW!!!: oldValue is the current data stored at address - may need to change later on
        // if(writeHookFunctions[address](value, data[address], address, mask) == true){
        if((*this->writeHookVector[address])(value, data[address], address, mask) == true){
            // std::cout << "Ok, the write hook function returned true, fam." << std::endl;
            return;
        }
    }
    // But in the case where we don't have a writeHookFunction for that location,
    // just hard-write the value at that location
    // std::cout << "No writeHooky :( at "<< int(address) << std::endl;
    this->data[address] = value;
}

u8 CPU::readData(u16 address){
    // Grab a reference to the readHookFunction
    readHookFunction hookPtr = this->readHookFunctions[address];

    // If we are past the general purpose I/O registers (we don't care about those) AND the hook at that address exists
    if(address >= 32 && hookPtr){
        // std::cout << "a readhook exists at location " << address << std::endl;
        /* digging in */
        // if(address == 110){
        //     std::cout << "-->there's a read hook for 110" << std::endl;
        // }

        return (*this->readHookFunctions[address])(address);
    }
    // Manually return the data
    // std::cout << "no readhook, will manually return, address is " << int(address) << std::endl;
    // if(address == 110){
    //     std::cout << "-->there's NO read hook for 110" << std::endl;
    // }
    return this->data[address];
}

/*
When watchdog reset triggered (aka when she be running for too long) - empty by default
*/
void CPU::onWatchdogReset() {}

/*
Returns SREG at data[95] (aka 0x5F, which matches the datasheet)
*/
u8 CPU::getSREG(){
    return this->data[95];
}

/*
Returns whether most signifiact bit of SREG is 1 --> means interrupts are enabled
*/
bool CPU::getInterruptsEnabled(){
    // Bitwise AND the SREG with 10000000 -> want to check MSB
    if ((this->getSREG() & 0x80) == 0x80) {
        return true;
    }
    return false;
}

void CPU::setInterruptFlag(AVRInterruptConfig *interrupt) {
    // std::cout << "Inside set interrupt flag" << std::endl;
    const u16 flagReg = interrupt->flagRegister;
    const u8 flagMsk = interrupt->flagMask;
    const u16 enReg = interrupt->enableRegister;
    const u8 enMsk = interrupt->enableMask;

    // If inverse flag is set
    if (interrupt->inverseFlag == true) {
        // AND the negation of flag mask and flag register contents and re-store it (inverts the bit of interest)
        this->data[flagReg] &= ~flagMsk;
    }
    else {
        // Store the flag register contents OR'd with the flag mask - set the flag bit
        this->data[flagReg] |= flagMsk;
    }
    // If the enable mask (bit of interest) matches the relevant bit of our enable register
    if ((this->data[enReg] & enMsk) == enMsk) {
        // Queue the interrupt
        this->queueInterrupt(interrupt);
    }
}

void CPU::updateInterruptsEnabled(AVRInterruptConfig* interrupt, u8 registerValue) {
    std::cout << "Inside updateInterruptsEnabled!" << std::endl;
    const u8 enableMask = interrupt->enableMask;
    const u16 flagRegister = interrupt->flagRegister;
    const u8 flagMask = interrupt->flagMask;
    const bool inverseFlag = interrupt->inverseFlag;

    std::cout << "Right now, our register value is: " << int(registerValue) << " and our enableMask is: " << int(enableMask) << " and so our conditional is checking: " << int(registerValue & enableMask) << std::endl;

    if (registerValue & enableMask) {
        const u8 bitSet = this->data[flagRegister] & flagMask;
        // std::cout << "--> the data at the flagRegister: " << int(this->data[flagRegister]) << " & the flagMask: " << int(flagMask) << " gives us the bitSet: " << int(bitSet) << " <---" << std::endl;
        if (inverseFlag ? !bitSet : bitSet) {
            std::cout << "Queueing the interrupt!" << std::endl;
            this->queueInterrupt(interrupt);
        }
        std::cout << "Skippin innit" << std::endl;
    } else {
        std::cout << "Clearing the interrupt!" << std::endl;
        this->clearInterrupt(interrupt, false);
    }
}

/* OLD updateInterruptsEnabled below VV*/

// void CPU::updateInterruptsEnabled(AVRInterruptConfig *interrupt, u8 registerValue) {

//     // extracts the enableMask, flagRegister, flagMask, inverseFlag from interrupt and saves them as consts
//     const u16 flagReg = interrupt->flagRegister;
//     const u8 flagMsk = interrupt->flagMask;
//     const bool inverse = interrupt->inverseFlag;
//     const u8 enMsk = interrupt->enableMask;
//     int bitSet = 0;

//     // IF registerValue & enableMask --> are 1?
//     if ((registerValue & enMsk) == enMsk) {
//         std::cout << "Enabled! Inside first IF" << std::endl;
//         // Then & the data[flagRegister] with flagMask, store in a const bitSet
//         bitSet = this->data[flagReg] & flagMsk;

//         if(inverse == true) {
//             // If !bitSet is true, bitSet is falsy (if it's 0b00000000)
//             if(bitSet == 0){
//                 std::cout << "Inverse true and bitSet is 0" << std::endl;
//                 this->queueInterrupt(interrupt);
//             }
//         }
//         // Otherwise, if inverse flag is false and bitSet is a number
//         else if (bitSet != 0) {
//             std::cout << "Inverse false and bitSet is not 0!" << std::endl;
//             this->queueInterrupt(interrupt);
//         }
//     }
//     else {
//         std::cout << "clearing interrupt and actually calling it this time lol" << std::endl;
//         this->clearInterrupt(interrupt, false);
//     }
// }

void CPU::queueInterrupt(AVRInterruptConfig *interrupt){

    // store address of interrupt as a const
    const u8 addr = interrupt->address;

    // store interrupt at pendingInterrupts[address]
    this->pendingInterrupts[addr] = interrupt;

    std::cout << "Pending interrupt at " << int(addr) << std::endl;

    // IF no interrupts are queued or nextInterrupt > address
    if ((this->nextInterrupt == -1) || (this->nextInterrupt > addr)) {
        // then nextInterrupt = address (it makes sense to queue it up next)
        std::cout << "Either this is the first interrupt queued or its address is less than the current next interrupt " << std::endl;
        this->nextInterrupt = addr;
    }
    // IF address is greater than maxInterrupt
    if (addr > maxInterrupt) {
        std::cout << "The address is > the current max interrupt address" << std::endl;
        // then max interrupt is assigned address
        maxInterrupt = addr;
    }      
}

void CPU::clearInterrupt(AVRInterruptConfig* interrupt, bool clearFlag) {
    const u8 address = interrupt->address;
    const u16 flagRegister = interrupt->flagRegister;
    const u8 flagMask = interrupt->flagMask;

    // Clear the flag if clearFlag is true
    if (clearFlag) {
        std::cout << "Clear flag true" << std::endl;
        this->data[flagRegister] &= ~flagMask;
    }

    // Access pending interrupts and max interrupt
    auto& pendingInterrupts = this->pendingInterrupts;
    const i16 maxInterrupt = this->maxInterrupt;

    // If no interrupt is pending at the address, return
    if (pendingInterrupts[address] == nullptr) {
        std::cout << "Interrupt not in the queue, returning" << std::endl;
        return;
    }

    // Remove the interrupt from the queue
    pendingInterrupts[address] = nullptr;

    // Update the next interrupt if the cleared interrupt was the next scheduled
    if (this->nextInterrupt == address) {
        this->nextInterrupt = -1; // Reset to no interrupt
        for (int i = address + 1; i <= maxInterrupt; i++) {
            if (pendingInterrupts[i] != nullptr) {
                this->nextInterrupt = i; // Set the next interrupt
                break;
            }
        }
    }
}

/* Old clear interrupt VVV */

// void CPU::clearInterrupt(AVRInterruptConfig *interrupt, bool clearFlag) {
//     // takes in the address, flagRegister, and flagMask of an interrupt, and clearFlag = true by default
//     const u16 flagReg = interrupt->flagRegister;
//     const u8 flagMsk = interrupt->flagMask;
//     const u8 addr = interrupt->address;
//     // IF clearFlag is true
//     if(clearFlag) {
//         std::cout << "Clear flag true" << std::endl;
//         // then data[flagRegister] gets &= with ~flagMask and stored back in it
//         this->data[flagReg] &= ~flagMsk;  
//         std::cout << "Data in flagReg &= ~flagMsk " << int(this->data[flagReg]) << std::endl;   
//     }
//     // Would extract pending interrupts here too, but for now don't worry about it
//     const i16 maxInt = this->maxInterrupt;
//     // IF no pending interrupts at address, return :(
//     if (this->pendingInterrupts[addr] == 0) {
//         std::cout << "Interrupt not in the queue, returning" << std::endl;
//         return;
//     }
//     // Clear the interrupt from the queue
//     std::cout << "Before removal: " << pendingInterrupts[addr] << std::endl;
//     this->pendingInterrupts[addr] = nullptr;
//     std::cout << "After removal: " << pendingInterrupts[addr] << std::endl;
//     // IF nextInterrupt is address
//     if (this->nextInterrupt == addr) {
//         // then nextInterrupt set to -1
//         nextInterrupt = -1;
//         // Search for the next pending interrupt, make it the next Interrupt
//         for (int i = addr + 1; i <= maxInterrupt; i++) {
//             if (pendingInterrupts[i]) {
//                 this->nextInterrupt = i;
//                 break;
//             }
//         }
//         std::cout << "Next interrupt: " << nextInterrupt << std::endl;
//     }
// }

void CPU::clearInterruptByFlag(AVRInterruptConfig *interrupt, u8 registerValue) {
    // Store flagRegister and flagMask as consts
    const u16 flagReg = interrupt->flagRegister;
    const u8 flagMsk = interrupt->flagMask;
    // IF registervalue & flagMask - note: The flagMask defines which bits are relevant for the check. If flagMask 
    // has certain bits set to 1, the condition will only be true if those same bits in registerValue are also 1.If 
    // the result of registerValue & flagMask is non-zero (i.e., at least one of the bits defined by flagMask is 
    // set in registerValue), the condition evaluates to true. If it's zero, it evaluates to false.
    if ((registerValue & flagMsk) == flagMsk){
        // Then we clear the value at the flag mask
        this->data[flagReg] &= ~flagMsk;
        std::cout << "Result: " << int(this->data[flagReg]) << std::endl;
        // clear the interrupt, no pass in for clearFlag
        this->clearInterrupt(interrupt);
    }
}

AVRClockEventCallback CPU::addClockEvent(AVRClockEventCallback callback, int cycles) {
    // Calculate absolute cycle time for the event
    cycles = this->cycles + std::max(1, cycles);

    // Get an available clock event entry from the pool or create a new one
    AVRClockEventEntry *entry;
    if (!this->clockEventPool.empty()) {
        entry = this->clockEventPool.back();
        this->clockEventPool.pop_back();
        // std::cout << "CLK:: REUSING" << std::endl;
    } else {
        entry = new AVRClockEventEntry();
        // std::cout << "CLK:: NEW CLOCK EVENT" << std::endl;
    }

    // Initialize the entry with the provided data
    entry->cyclesForEvent = cycles;
    entry->callbackFunc = callback;
    if(entry->callbackFunc == nullptr) {
        std::cout << "nullllll" <<std::endl;
    }
    // std::cout << "STARTING CALLING THE CALLBACK FUNC IN ENTRY" << std::endl;
    // (*entry->callbackFunc)();
    // std::cout << "DONE CALLING THE CALLBACK FUNC IN ENTRY" << std::endl;
    entry->next = nullptr;

    // Find the insertion point in the sorted linked list
    AVRClockEventEntry *clockEvent = this->nextClockEvent;
    AVRClockEventEntry *lastItem = nullptr;

    while (clockEvent && clockEvent->cyclesForEvent < cycles) {
        lastItem = clockEvent;
        clockEvent = clockEvent->next;
    }

    // Insert the new entry in the appropriate position
    if (lastItem) {
        lastItem->next = entry;
        entry->next = clockEvent;
    } else {
        this->nextClockEvent = entry;
        entry->next = clockEvent;
    }

    return callback;
}

/*** ADD CLOCK EVENT ***/

// AVRClockEventCallback CPU::addClockEvent(AVRClockEventCallback callback, int numCycles) {
//     // Will use the clockEventPool instead of creating a separate reference to it here
//     // Absolute cycle time until callback function is executed
//     numCycles = this->cycles + std::max(1, numCycles);
//     // ~~ Basically, if there's a pooled clock event, make it the next entry, otherwise create a new one ~~
//     AVRClockEventEntry *entry;
//     // const entry - of type AVRClockEventEntry which *might* be assigned maybeEntry, or if it's null, assign it
//     if(clockEventPool.size() > 1) {
//         // Grabbing the last element of the vector to see if it's a reusable clock event
//         // This line causing seg fault
//         AVRClockEventEntry *maybeEntry = this->clockEventPool.back();
//         entry = maybeEntry;
//     }
//     // Returned null, need to create a new event entry
//     else {
//         // Passing the
//         entry = new AVRClockEventEntry();
//     }
//     // set cycles of entry = to cycles
//     entry->cyclesForEvent = numCycles;
//     // set callback of entry = to callback
//     entry->callbackFunc = callback;
//     entry->next = nullptr;
//     // Extracting CPU's nextClockEvent as nextClockEvent
//     AVRClockEventEntry *clockEvent =  this->nextClockEvent;
//     // let lastItem = null
//     AVRClockEventEntry *lastEvent = nullptr;
//     // WHILE clockEvent AND (&&) clockEvent.cycles < cycles - loop through the clock events and update the "next" event they point to
//     while (clockEvent && clockEvent->cyclesForEvent < numCycles) {
//         // lastItem = clockEvent
//         lastEvent = clockEvent;
//         // clockEvent gets the next clockEvent
//         clockEvent = clockEvent->next;
//     }
//     // ~~ Finding the insertion point ~~
//     // If the last event is a valid entry
//     if(lastEvent != nullptr) {
//         // lastItem.next gets assigned entry
//         lastEvent->next = entry;
//         // entry.next is assigned clockEvent
//         entry->next = clockEvent;
//     }
//     // Otherwise, it's the first entry in the vector
//     else {
//         // nextClockEvent gets assigned entry
//         this->nextClockEvent = entry;
//         // entry.next gets assigned clockEvent
//         entry->next = clockEvent;
//     }
//     return callback;
// }

/*** UPDATE CLOCK EVENT ***/

bool CPU::updateClockEvent(AVRClockEventCallback callback, int cycles) {
    // First, clear the existing clock event associated with the callback
    if (this->clearClockEvent(callback)) {
        // If successful, add the updated clock event with the new cycle count
        this->addClockEvent(callback, cycles);
        return true;
    }
    // Return false if the event was not cleared
    return false;
}

// bool CPU::updateClockEvent(AVRClockEventCallback callback, int cycles) {
//     // If we successfully clear the old clock event associated with this callback func
//     if(this->clearClockEvent(callback)) {
//         // Then let's update with appropriate number of cycles
//         this->addClockEvent(callback, cycles);
//         return true;
//     }
//     return false;
// }

/*** CLEAR CLOCK EVENT ***/

bool CPU::clearClockEvent(AVRClockEventCallback callback) {
    AVRClockEventEntry* clockEvent = this->nextClockEvent;
    if (!clockEvent) {
        return false;
    }

    AVRClockEventEntry* lastItem = nullptr;

    while (clockEvent) {
        if (clockEvent->callbackFunc == callback) {
            if (lastItem) {
                lastItem->next = clockEvent->next;
            } else {
                this->nextClockEvent = clockEvent->next;
            }

            // Return the entry to the pool if the pool has space
            if (this->clockEventPool.size() < 10) {
                this->clockEventPool.push_back(clockEvent);
            }

            return true;
        }

        lastItem = clockEvent;
        clockEvent = clockEvent->next;
    }

    return false;
}

// bool CPU::clearClockEvent(AVRClockEventCallback callback) {
//     // extracting CPU's nextClockEvent as clockEvent
//     // Extracting CPU's nextClockEvent as nextClockEvent
//     AVRClockEventEntry *clockEvent =  this->nextClockEvent;
//     // IF NOT clockEvent
//     if (clockEvent == nullptr) {
//         return false;
//     }
//     // Will use this->clockEventPool in this function
//     // let lastItem = null
//     AVRClockEventEntry *lastEvent = nullptr;
//     // WHILE clockEvent returns true
//     while (clockEvent) {
//         // std::cout << "STUCK" << std::endl;
//         // IF clockEvent's callback = our callback
//         if (clockEvent->callbackFunc == callback) {
//             std::cout << "TRIGGERED CLEAR CLK" << std::endl;
//             // IF lastItem exists
//             if(lastEvent != nullptr) {
//                 // lastItem.next = clockEvent.next
//                 lastEvent->next = clockEvent->next;
//             }
//             // ELSE
//             else {
//                 // this.nextClockEvent = clockEvent.next
//                 this->nextClockEvent = clockEvent->next;
//             }
//             // IF length of clockEventPool < 10
//             if (this->clockEventPool.size() < 10){
//                 // push clockEvent onto the clockEventPool
//                 this->clockEventPool.push_back(clockEvent);
//             }
//             // Successfully updated pointers
//             return true;
//         }
//         // Move down the linked list
//         lastEvent = clockEvent;
//         clockEvent = clockEvent->next;
//     }
//     // Didn't find a match
//     return false;
// }

/*** TICK ***/

void CPU::tick() {
    // Handle the next clock event
    AVRClockEventEntry* nextClockEvent = this->nextClockEvent;

    if (nextClockEvent && nextClockEvent->cyclesForEvent <= this->cycles) {
        // Execute the callback function
        // std::cout << "STARTING CALLING THE CALLBACK FUNC" << std::endl;
        (*nextClockEvent->callbackFunc)();

        // Advance to the next clock event in the list
        this->nextClockEvent = nextClockEvent->next;

        // Return the processed event to the pool if space is available
        if (this->clockEventPool.size() < 10) {
            this->clockEventPool.push_back(nextClockEvent);
        }
    }

    // Handle the next interrupt
    int nextInterrupt = this->nextInterrupt;
    if (this->getInterruptsEnabled() && nextInterrupt >= 0) {
        // Get the interrupt configuration
        AVRInterruptConfig* interrupt = this->pendingInterrupts[nextInterrupt];

        if (interrupt) {
            // Trigger the interrupt
            avrInterrupt(this, interrupt->address);

            // Clear the interrupt if it's not constant
            if (!interrupt->constant) {
                this->clearInterrupt(interrupt);
            }
        }
    }
}

// void CPU::tick() {

//     // ~~ Handling next clock event ~~
 
//     // get cpu's next clock event and store as a const nextClockEvent
//     AVRClockEventEntry *nextClk = this->nextClockEvent;

//     // IF (nextClockEvent && nextClockEvent.cycles <= this.cycles)
//     if (nextClk && nextClk->cyclesForEvent <= this->cycles){
//         // std::cout << "made it here"<< std::endl;
//         // call the callback function on nextClockEvent (should return void)
//         (*nextClk->callbackFunc)();
//         // std::cout << "called it back"<< std::endl;
//         // this.nextClockEvent = nextClockEvent.next
//         this->nextClockEvent = nextClk->next;

//         // IF length of clockEventPool < 10
//         if (this->clockEventPool.size() < 10) {
//             // push nextClockEvent onto the clockEventPool
//             this->clockEventPool.push_back(nextClk);
//         }            
//     }

//     // ~~ Handling next interrupt ~~

//     // Extracting CPU's nextInterrupt as nextInterrupt
//     // Skip for now

//     // IF CPU's interrupts are enabled AND nextInterrupt >= 0
//     if (this->getInterruptsEnabled() && this->nextInterrupt >= 0) {

//         // declare a const, interrupt, and assign it this.pendingInterrupts[nextInterrupt]
//         AVRInterruptConfig *interrupt = pendingInterrupts[nextInterrupt];

//         // call avrInterrupt and pass it the CPU and interrupt.address
//         avrInterrupt(this, interrupt->address);

//         // ~~~ DELETE NEXT LINE: FOR TESTING PURPOSES ONLY: calling the fake interrupt ~~~ 
//         // this->fakeISRAndRETI();

//         // IF interrupt is not constant
//         if (!interrupt->constant) {
//             // then clear the interrrupt
//             this->clearInterrupt(interrupt);
//        }
//     }
// }


void CPU::fakeISRAndRETI() {

    // Emergency routine that must be executed!!
    int a = 1;
    int b = 2;
    int c = a + b;

    // Increment to represent how long this function took (v fake)
    this->cycles++;

    u16 i = 0;

    // Get the return address from the interrupt
    if (this->pc22Bits) {
        i = this->getSP() + 3;
    }
    else {
        i = this->getSP() + 2;
    }

    // Set the SP to the return address
    this->setSP(i);

    // Removing the -1 for now, not relevant for our purposes
    this->PC = ((this->data[i-1] << 8) + (this->data[i]));

    if (pc22Bits) {
    this->PC |= this->data[i - 2] << 16;
    }

    /*   } else if (opcode === 0x9518) {
      RETI, 1001 0101 0001 1000 
      const { pc22Bits } = cpu;
      const i = cpu.dataView.getUint16(93, true) + (pc22Bits ? 3 : 2);

      cpu.dataView.setUint16(93, i, true);

      cpu.pc = (cpu.data[i - 1] << 8) + cpu.data[i] - 1;
      if (pc22Bits) {
        cpu.pc |= cpu.data[i - 2] << 16;
      }
    */
}

// int main(){
    // std::vector<u16> testPM(1024);

    // CPU *cpu = new CPU(testPM);
    // int x=2;
    // int y=3;
    // std::cout << "here" << std::endl;
    // std::cout << int(cpu->writeHookVector.size()) << std::endl;

    // cpu->writeHookVector.emplace(cpu->writeHookVector.begin()+5,([x](u8 test, u8 test2, u16 test3, u8 test4){return true;}));
    // cpu->writeHookVector.emplace(cpu->writeHookVector.begin()+6,([y](u8 test, u8 test2, u16 test3, u8 test4){return true;}));

    // std::cout << "here 2" << std::endl;

    // for(int i = 0; i<10; i++){
        
    //     std::cout << &cpu->writeHookVector[i] << std::endl;
    // }

// }