// Global consts and variables

// Shorthands for different types
typedef unsigned char u8;
typedef unsigned short u16;

/*
 A constant to represent the size of the register space (registerSpace) 
 (data memory minus internal SRAM, so 32 gen purp registers, 
 64 i/o registers, 160 extended i/o registers) -> size = 0x100 = 256
*/
const int REGISTER_SPACE = 0x100;
/*
 A constant for the maximum number of interrupts allowed - 128
 is sufficient
*/
const int MAX_INTERRUPTS = 128;

// Memory hook system for writing data to memory
/*
 An array of function pointers - each will handle different functionality around writing data to a certain location. We will define different functions for different parts of the microcontroller - e.g., different writeHook functions for different I/O ports.

 May need to add functionality for typedefs that return void, but this should work. If the specific writeHook function writes data, returns true, if not, returns false
*/
typedef bool (*writeHookFunction) (u8 value, u8 oldValue, u16 address, u8 mask);

// Memory hook system for reading data from memory
/*
  An array of function pointers - each will handle different functionality around reading data from a certain location. We will define different functions for different parts of the microcontroller - e.g., different read Hook functions for different timer configs.
*/
typedef u8 (*readHookFunction) (u16 address);

/*
 A class for configuring AVR interrupts
*/
class AVRInterrupt{
    u8 address;
    u16 enableRegister;
    u8 enableMask;
    u16 flagRegister;
    u8 flagMask;
    // constant?: boolean;
    // These should technically be bool OR null...fix
    bool constant;
    // inverseFlag?: boolean;
    // These should technically be bool OR null...fix
    bool inverseFlag;

};

class CPU {

    public:

    /*
    Internal data, stored in 8-bit chunks
    */
    u8 data[8192 + REGISTER_SPACE];

    /*
    Internal data, stored in 16-bit chunks
    */
    // Ignore for now, doesn't appear to be used

    /*
    creating a DataView object, and pass it in the "data" buffer (the complete binary string stored in "data") - The DataView view provides a low-level interface for reading and writing multiple number types in a binary ArrayBuffer, without having to care about the platform's endianness.

    For now, we will manually return two bytes of data in little-endian format: https://en.wikipedia.org/wiki/Endianness
    BYTE OFFSET is the address of the LOWER BYTE
    BYTE OFFSET + 1 should be the UPPER BYTE
    */
    u16 getUint16LittleEndian(int byteOffset);
    void setUint16LittleEndian(int byteOffset, u8 value);
 
    /*
    Array of write hook and read hook function pointers - size is # of SRAM bytes + REGISTER_SPACE, same size as data array (?)
    */
    writeHookFunction writeHookFunctions[8192 + REGISTER_SPACE];
    readHookFunction readHookFunctions [8192 + REGISTER_SPACE];

    /*
    Function for writing data
    */
    void writeData(u16 address, u8 value, u8 mask = 0xff);
    // Matches params of writeHooks
    // void writeData(u8 value, u8 oldValue, u16 address, u8 mask);

    /*
    Function for reading data
    */
    u8 readData(u16 address);
};