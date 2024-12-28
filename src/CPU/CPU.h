#include <iostream>
#include <functional>
#include <variant>
#include <vector>

#pragma once 

struct AVRPortConfig;

class AVRIOPort;

// Global consts and variables

// Shorthands for different types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef int16_t i16;
typedef int8_t i8;

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

class CPU;

// Memory hook system for writing data to memory
/*
 An array of function pointers - each will handle different functionality around writing data to a certain location. We will define different functions for different parts of the microcontroller - e.g., different writeHook functions for different I/O ports.

 May need to add functionality for typedefs that return void, but this should work. If the specific writeHook function writes data, returns true, if not, returns false
*/
// typedef bool (*writeHookFunction) (u8 value, u8 oldValue, u16 address, u8 mask);
typedef std::shared_ptr<std::function<bool(u8, u8, u16, u8)>> writeHookFunction;



// Memory hook system for reading data from memory
/*
  An array of function pointers - each will handle different functionality around reading data from a certain location. We will define different functions for different parts of the microcontroller - e.g., different read Hook functions for different timer configs.
*/
// typedef u8 (*readHookFunction) (u16 address);
typedef std::shared_ptr<std::function<u8(u16)>> readHookFunction;

/*
 A struct for configuring AVR interrupts
*/
struct AVRInterruptConfig {
    public:
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

/*
A type of function that acts as a "callback" function for clock events
*/
// typedef void (*AVRClockEventCallback) ();
typedef std::shared_ptr<std::function<void(void)>> AVRClockEventCallback;

/*
A struct representing a clock event entry
*/
struct AVRClockEventEntry {
  // Number of cycles needed for the event (?)
  int cyclesForEvent;
  // callback: AVRClockEventCallback;
  AVRClockEventCallback callbackFunc;
  // next: AVRClockEventEntry | null;
  AVRClockEventEntry *next;
};


class CPU {

  public:

  // Program memory
  std::vector<u16> programMemory;

  // Program memory stored in byte-sized chunks
  std::vector<u8> programBytes;

  // size of SRAM in bytes
  int SRAM_BYTES = 8192;

  // Function for setting stack pointer (SP) value
  void setSP(u16 value);
  // Function for getting SP value
  u16 getSP();

  // Program Counter
  u32 PC = 0;

  // For tracking clock cycles
  u32 cycles = 0;

  // Default contructor - note that for this file SRAM size is const
  CPU(std::vector<u16> progMem, int SRAMSize = 8192);

  // Reset function
  void reset();

  /*
  Internal data, stored in 8-bit chunks - note fixed SRAM size
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
  void setUint16LittleEndian(int byteOffset, u16 value);

  void setInt16LittleEndian(u16 address, i16 value);

  /*
  Array of write hook and read hook function pointers - size is # of SRAM bytes + REGISTER_SPACE, same size as data array (?)
  */
  // writeHookFunction writeHookFunctions[8192 + REGISTER_SPACE];
  // XXX Transitioning to a writeHookVector
  std::vector<writeHookFunction> writeHookVector;

  std::vector<readHookFunction> readHookFunctions;

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

  /*
  Whether the program counter (PC) can address 22 bits (the default is 16)
  */
  bool pc22Bits = false; 

  /*
  GPIO initialization
  */
  // creating a collection ("Set") of type AVRIOPort called gpioPorts
  std::vector<AVRIOPort *> GPIOPorts;

  // Creating an empty array of type AVRIOPort called gpioByPort
  AVRIOPort *GPIOByPort[255];

  // 16-bit signed integer to track nextInterrupt (initialized to -1)
  i16 nextInterrupt = -1;

  // 16-bit signed integer to track maxInterrupt (initialized to 0)
  i16 maxInterrupt = 0;

  void onWatchdogReset();

  /*
  Returns contents of the status Register, data[95] (aka 0x5F, which matches the datasheet)
  */
  u8 getSREG();

  /*
  Returns contents of the status Register, data[95] (aka 0x5F, which matches the datasheet)
  */
  bool getInterruptsEnabled();

  /*
  Function for setting interrupt flags!!
  */
  void setInterruptFlag(AVRInterruptConfig *interrupt);

  /*
  Function for updating whether interrupts are enabled
  */
  void updateInterruptsEnabled(AVRInterruptConfig *interrupt, u8 registerValue);

  /*
  Function for queueing interrupts - make bool?
  */
  void queueInterrupt(AVRInterruptConfig *interrupt);
  
  /*
  Function for clearing a queued interrupt - make bool?
  */
  void clearInterrupt(AVRInterruptConfig *interrupt, bool clearFlag = true);
  
  /*
  Function for clearing a queued interrupt by its flag - make bool?
  */
  void clearInterruptByFlag(AVRInterruptConfig *interrupt, u8 registerValue); // regVal might be int?
  
  // Change back to private after testing
  public:

  /*
  Array of pending interrupts
  */
  AVRInterruptConfig* pendingInterrupts[MAX_INTERRUPTS] = {nullptr};

  /*
  A pointer to the next clock event
  */
  AVRClockEventEntry *nextClockEvent;

  /*
  An array of clock event entries
  */
  std::vector<AVRClockEventEntry *> clockEventPool;

  public:

  /*
  Function for adding a clock event
  */
  AVRClockEventCallback addClockEvent(AVRClockEventCallback callback, int cycles);

  /*
  Function for updating a clock event
  */
  bool updateClockEvent(AVRClockEventCallback callback, int cycles);

  /*
  Function for clearing a clock event
  */
  bool clearClockEvent(AVRClockEventCallback callback);

  /*
  Function for the "ticking" of the clock
  */
  void tick();

  /*
  A test function to fake an interrupt service routine + return
  */
  void fakeISRAndRETI();

   
};