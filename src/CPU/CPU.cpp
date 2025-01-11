#include "CPU.h"
#include "interrupt.h"

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  CPP file for the CPU module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

CPU::CPU(std::vector<u16> progMem, int SRAMSize){

    // Filling data[] with zeros
    std::fill(data, data + dataArraySize, 0);
    // Initializing SRAM_BYTES
    this->SRAM_BYTES = SRAMSize;
    // Set up the writeHookVector and readHookFunctions with the correct size and fill with nullptr
    this->writeHookVector = std::vector<std::shared_ptr<std::function<bool(u8, u8, u16, u8)>>>(8448, nullptr);
    this->readHookFunctions = std::vector<readHookFunction>(8448, nullptr);
    // STEP 1: fill program memory with contents of progMem
    for(int i = 0; i < progMem.size(); i++) {
        this->programMemory.push_back(progMem[i]);
    }
    // STEP 2: Fill programBytes with the contents of programMemory but in 8-bit chunks
    programBytes.resize(programMemory.size() * 2);
    for (int i = 0; i < programMemory.size(); i++) {
        // Fill the array in LITTLE-ENDIAN style (least-significant byte at lower address)
        programBytes[2 * i] = progMem[i] & 0xFF;            // Lower 8 bits
        programBytes[2 * i + 1] = (progMem[i] >> 8) & 0xFF; // Upper 8 bits
    }
    // Updating 22 bit addressing based on size of programBytes
    this->pc22Bits = this->programBytes.size() > 0x20000 ? true : false;
    // Finally, reset the system!
    this->reset();
}

void CPU::reset(){
    // Reset stack pointer
    setSP(SRAM_BYTES + REGISTER_SPACE - 1);
    // Reset program counter
    this->PC = 0;
    // Fill pending interrupts with nullptr - test
    std::fill(pendingInterrupts, (pendingInterrupts + MAX_INTERRUPTS), nullptr);
    // Reset the next interrupt
    nextInterrupt = -1;
    // Reset the next clock event
    this->nextClockEvent = nullptr;
}

void CPU::setSP(u16 value) {
    setUint16LittleEndian(93, value);
}

u16 CPU::getSP() {
    return getUint16LittleEndian(93);
}

u16 CPU::getUint16LittleEndian(int byteOffset){
    
    // Get lower 8 bits first, shift them over, then get upper 8 bits
    u16 value = (this->data[byteOffset] & 0xFF) | ((this->data[byteOffset+1]) << 8);
    
    return value;
}

void CPU::setUint16LittleEndian(int byteOffset, u16 value){
    
    this->data[byteOffset] = value & 0xFF; // Lower 8 bits
    this->data[byteOffset+1] = (value >> 8) & 0xFF; // Upper 8 bits
}

void CPU::setInt16LittleEndian(u16 address, i16 value) {
    this->data[address] = value & 0xFF;         // Lower byte
    this->data[address + 1] = (value >> 8) & 0xFF; // Upper byte
}

void CPU::writeData(u16 address, u8 value, u8 mask){

    // Grabbing a pointer to a given function
    auto hookPtr = this->writeHookVector[address];

    // If hookPtr exists
    if(hookPtr != nullptr) {
        // If the write hook executes correctly, we're done!
        if((*this->writeHookVector[address])(value, data[address], address, mask) == true){
            return;
        }
    }
    // But in the case where we don't have a writeHookFunction for that location,
    // just hard-write the value at that location
    this->data[address] = value;
}

u8 CPU::readData(u16 address){
    // Grab a reference to the readHookFunction
    readHookFunction hookPtr = this->readHookFunctions[address];

    // If we are past the general purpose I/O registers (we don't care about those) AND the hook at that address exists
    if(address >= 32 && hookPtr){
        return (*this->readHookFunctions[address])(address); // Then we done
    }
    // Otherwise, manually return the data
    return this->data[address];
}

void CPU::onWatchdogReset() {}

u8 CPU::getSREG(){
    return this->data[95];
}

bool CPU::getInterruptsEnabled(){
    // Bitwise AND the SREG with 10000000 -> want to check most signifiact bit (MSB)
    if ((this->getSREG() & 0x80) == 0x80) {
        return true;
    }
    return false;
}

void CPU::setInterruptFlag(AVRInterruptConfig *interrupt) {
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
    const u8 enableMask = interrupt->enableMask;
    const u16 flagRegister = interrupt->flagRegister;
    const u8 flagMask = interrupt->flagMask;
    const bool inverseFlag = interrupt->inverseFlag;
    // If we get a nonzero byte when applying the enable mask
    if (registerValue & enableMask) {
        // Then we apply the flag mask to the data
        const u8 bitSet = this->data[flagRegister] & flagMask;
        // If the bit set checks out (depending on inverse flag), then we queue the interrupt
        if (inverseFlag ? !bitSet : bitSet) {
            this->queueInterrupt(interrupt);
        }
    } else {
        // Otherwise, we clear the interrupt
        this->clearInterrupt(interrupt, false);
    }
}

void CPU::queueInterrupt(AVRInterruptConfig *interrupt){
    // Store address of interrupt as a const
    const u8 addr = interrupt->address;
    // Store interrupt at pendingInterrupts[address]
    this->pendingInterrupts[addr] = interrupt;
    // IF no interrupts are queued or nextInterrupt > address
    if ((this->nextInterrupt == -1) || (this->nextInterrupt > addr)) {
        // Then nextInterrupt = address (it makes sense to queue it up next)
        this->nextInterrupt = addr;
    }
    // IF address is greater than maxInterrupt
    if (addr > maxInterrupt) {
        // Then max interrupt is assigned address
        maxInterrupt = addr;
    }      
}

void CPU::clearInterrupt(AVRInterruptConfig* interrupt, bool clearFlag) {
    const u8 address = interrupt->address;
    const u16 flagRegister = interrupt->flagRegister;
    const u8 flagMask = interrupt->flagMask;
    // Clear the flag if clearFlag is true
    if (clearFlag) {
        this->data[flagRegister] &= ~flagMask;
    }
    // Access pending interrupts and max interrupt
    auto& pendingInterrupts = this->pendingInterrupts;
    const i16 maxInterrupt = this->maxInterrupt;
    // If no interrupt is pending at the address, return
    if (pendingInterrupts[address] == nullptr) {
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

void CPU::clearInterruptByFlag(AVRInterruptConfig *interrupt, u8 registerValue) {
    // Store flagRegister and flagMask as consts
    const u16 flagReg = interrupt->flagRegister;
    const u8 flagMsk = interrupt->flagMask;
    // If registervalue & flagMask - note: The flagMask defines which bits are relevant for the check. If flagMask 
    // has certain bits set to 1, the condition will only be true if those same bits in registerValue are also 1.
    // If the result of registerValue & flagMask is non-zero (i.e., at least one of the bits defined by flagMask is 
    // set in registerValue), the condition evaluates to true. If it's zero, it evaluates to false.
    if ((registerValue & flagMsk) == flagMsk){
        // Then we clear the value at the flag mask
        this->data[flagReg] &= ~flagMsk;
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
    } else {
        entry = new AVRClockEventEntry();
    }

    // Initialize the entry with the provided data
    entry->cyclesForEvent = cycles;
    entry->callbackFunc = callback;
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
    // Return the callback function
    return callback;
}

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

bool CPU::clearClockEvent(AVRClockEventCallback callback) {
    // Grab the next clock event
    AVRClockEventEntry* clockEvent = this->nextClockEvent;
    // If it's null, then return false
    if (!clockEvent) {
        return false;
    }
    // Pointer to the last clock event
    AVRClockEventEntry* lastItem = nullptr;
    // While we have a valid clock event, check for a matching callback. If we find it, Remove the clock event of interest from the linked list
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
        // Shift down the list
        lastItem = clockEvent;
        clockEvent = clockEvent->next;
    }
    return false;
}

void CPU::tick() {
    // Handle the next clock event
    AVRClockEventEntry* nextClockEvent = this->nextClockEvent;
    if (nextClockEvent && nextClockEvent->cyclesForEvent <= this->cycles) {
        // Execute the callback function
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
}