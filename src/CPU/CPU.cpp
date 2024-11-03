#include "CPU.h"
#include <iostream>

u16 CPU::getUint16LittleEndian(int byteOffset){
    // TO DO: add functionality
    return 9;
}

void CPU::setUint16LittleEndian(int byteOffset, u8 value){
    // TO DO: add functionality
}

// typedef bool (*writeHookFunction) (u8 value, u8 oldValue, u16 address, u8 mask);
void CPU::writeData(u16 address, u8 value, u8 mask){

    // Grabbing a pointer to a given function
    writeHookFunction hookPtr = writeHookFunctions[address];
    // If hookPtr exists
    if(hookPtr) {
        std::cout << "a hook exists at location " << address << std::endl;
        // If, by calling the writeHookFunction, we write the value at the location,
        // then we are already done!
        // !!!FOR NOW!!!: oldValue is the current data stored at address - may need to change later on
        if(writeHookFunctions[address](value, data[address], address, mask) == true){
            std::cout << "Ok, the write hook function returned true, fam." << std::endl;
            return;
        }
    }
    // But in the case where we don't have a writeHookFunction for that location,
    // just hard-write the value at that location
    // std::cout << "should not be reaching here" << std::endl;
    this->data[address] = value;
}

u8 CPU::readData(u16 address){
    // Grab a reference to the readHookFunction
    readHookFunction hookPtr = this->readHookFunctions[address];

    // If we are past the general purpose I/O registers (we don't care about those) AND the hook at that address exists
    if(address >= 32 && hookPtr){
        std::cout << "a readhook exists at location " << address << std::endl;
        return this->readHookFunctions[address](address);
    }
    // Manually return the data
    std::cout << "no readhook, will manually return" << std::endl;
    return this->data[address];
}