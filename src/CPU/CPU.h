#include <iostream>
#include <functional>
#include <variant>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <cstdint>
#pragma once

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  Header file for the CPU module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

/* We'll be using AVRPortConifg and AVRIOPort in this module */
struct AVRPortConfig;
class AVRIOPort;

/* Shorthands for different types */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef int16_t i16;
typedef int8_t i8;
/*
  A type of function called writeHookFunction. Each will handle different functionality around writing data to a certain location. We will define different functions for different parts of the microcontroller - e.g., different writeHook functions for different I/O ports or special registers.
*/
typedef std::shared_ptr<std::function<bool(u8, u8, u16, u8)>> writeHookFunction;
// OLD --> typedef bool (*writeHookFunction) (u8 value, u8 oldValue, u16 address, u8 mask);

/*
  A type of function called readHookFunction. Each will handle different functionality around reading data from a certain location. We will define different functions for different parts of the microcontroller - e.g., different read Hook functions for different timer configurations.
*/
typedef std::shared_ptr<std::function<u8(u16)>> readHookFunction;
// OLD -->typedef u8 (*readHookFunction) (u16 address);

/* A type of function that acts as a "callback" function for clock events */
typedef std::shared_ptr<std::function<void()>> AVRClockEventCallback;
// OLD --> typedef void (*AVRClockEventCallback) ();

/* How long a clock cycle takes in nanoseconds (technically, 62.5 for an Uno board) */
const int cycleTime = 63;

/*
  A constant to represent the size of the register space 
  (data memory minus internal SRAM, so 32 general purpose registers, 
  64 i/o registers, 160 extended i/o registers) -> size = 0x100 = 256
*/
const int REGISTER_SPACE = 0x100;
/*
 A constant for the maximum number of interrupts allowed - 128 is sufficient
*/
const int MAX_INTERRUPTS = 128;



/* The size of our data array */
constexpr size_t dataArraySize = 8192 + REGISTER_SPACE;

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
    // Note: constant should technically be bool OR null
    bool constant;
    // Note: inverseFlag should technically be bool OR null
    bool inverseFlag = false;
};


/* A struct representing a clock event entry */
struct AVRClockEventEntry {
  // Number of cycles needed for the event
  int cyclesForEvent;
  AVRClockEventCallback callbackFunc;
  AVRClockEventEntry *next;
};

/* --- And now, what we've all been waiting for: the CPU class --- */
class CPU {
  // Since our use of this system is pretty limited for now, we are keeping it public
  public:

  // Vector of write hook functions 
  std::vector<writeHookFunction> writeHookVector;
  // OLD --> writeHookFunction writeHookFunctions[8192 + REGISTER_SPACE];

  // Vector of read hook functions
  std::vector<readHookFunction> readHookFunctions;

  // Program memory
  std::vector<u16> programMemory;
  // Program memory but stored in byte-sized chunks
  std::vector<u8> programBytes;
  // size of SRAM in bytes
  int SRAM_BYTES = 8192;

  /*
    Function for setting stack pointer (SP) value.
    @param value: The location at which you would like to point stack pointer.
    @returns No return value 
  */
  void setSP(u16 value);

  /* 
    Function for getting the stack pointer value.
    @returns A 16-bit unsigned int that represents the location to which the SP is pointing.
    
  */
  u16 getSP();

  // Program Counter
  u32 PC = 0;

  // For tracking clock cycles
  u32 cycles = 0;

  /* Default contructor - note that for this file SRAM size is const  */
  CPU(std::vector<u16> progMem, int SRAMSize = 8192);

  /* 
    Reset function
    - Points stack pointer to last data memory location
    - Resets program counter
    - Empty pending interrupts
    - Reset next interrupt
    - Reset next clock event
  */
  void reset();

  /* Internal data, stored in 8-bit chunks - note fixed SRAM size */
  u8 data[dataArraySize];

  /* Internal data, stored in 16-bit chunks */
  // Ignore for now, doesn't appear to be used

  /* 
    Getter for a 16-bit value. For now, we will manually return the two bytes of data in little-endian format: https://en.wikipedia.org/wiki/Endianness. Note BYTE OFFSET is the address of the LOWER BYTE, BYTE OFFSET + 1 is the UPPER BYTE.
    @param byteOffset: Location in memory you'd like to retrieve (lower byte)
    @returns The value stored at byteOffset and byteOffset+1
  */
  u16 getUint16LittleEndian(int byteOffset);

  /*
    Setter for a 2-byte UNSIGNED value.
    @param byteOffset: Location in memory at which you'd like to store the value (lower byte)
    @param value: The value you're trying to store
  */
  void setUint16LittleEndian(int byteOffset, u16 value);

  /*
    Setter for a 2-byte SIGNED value.
    @param byteOffset: Location in memory at which you'd like to store the value (lower byte)
    @param value: The value you're trying to store (signed)
  */
  void setInt16LittleEndian(u16 address, i16 value);

  /*
    Function for writing data.
    @param address: Where are you writing to?
    @param value: What are you trying to write?
    @param mask: Are you masking for specific bits?
    @returns No value returned
  */
  void writeData(u16 address, u8 value, u8 mask = 0xff);
  // Matches params of writeHooks
  // void writeData(u8 value, u8 oldValue, u16 address, u8 mask);

  /*
    Function for reading data.
    @param address: Where are you reading from?
    @returns The byte that you're trying to read
  */
  u8 readData(u16 address);

  // Whether the program counter (PC) can address 22 bits (the default is 16)
  bool pc22Bits = false; 

  /* --- GPIO stuff! --- */

  // Creating a vector of type AVRIOPort called gpioPorts
  std::vector<AVRIOPort *> GPIOPorts;

  // Creating an empty Array of type AVRIOPort called gpioByPort
  AVRIOPort *GPIOByPort[255];

  // 16-bit signed integer to track nextInterrupt (initialized to -1)
  i16 nextInterrupt = -1;

  // 16-bit signed integer to track maxInterrupt (initialized to 0)
  i16 maxInterrupt = 0;

  /* Function onWatchdogReset currently not implemented! */
  void onWatchdogReset();

  /*
    @returns Contents of the status Register (SREG), data[95] (aka 0x5F, which matches the datasheet)
  */
  u8 getSREG();

  /*
    @returns Whether interrupts are currently enabled, according to SREG
  */
  bool getInterruptsEnabled();

  /*
    Function for setting interrupt flags
    @param interrupt: A pointer to an AVRInterruptConfig.
    @returns No return value
  */
  void setInterruptFlag(AVRInterruptConfig *interrupt);

  /*
    Function for updating whether interrupts are enabled
    @param interrupt: A pointer to an AVRInterruptConfig
    @param registerValue: The byte you're checking against the enable mask
    @returns No return value
  */
  void updateInterruptsEnabled(AVRInterruptConfig *interrupt, u8 registerValue);

  /*
    Function for queueing interrupts
    @param interrupt: A pointer to an AVRInterruptConfig
    @returns No return value
  */
  void queueInterrupt(AVRInterruptConfig *interrupt);
  
  /*
    Function for clearing a queued interrupt
    @param interrupt: A pointer to an AVRInterruptConfig
    @param clearFlag: Bool for whether the flag register should be cleared or not
    @returns No return value
  */
  void clearInterrupt(AVRInterruptConfig *interrupt, bool clearFlag = true);
  
  /*
    Function for clearing a queued interrupt by its flag
    @param interrupt: A pointer to an AVRInterruptConfig
    @param registerValue: The byte you're checking against the flag mask
    @returns No return value
  */
  void clearInterruptByFlag(AVRInterruptConfig *interrupt, u8 registerValue);

  // Array of pending interrupts
  AVRInterruptConfig* pendingInterrupts[MAX_INTERRUPTS] = {nullptr};

  // A pointer to the next clock event
  AVRClockEventEntry *nextClockEvent;

  // A vector of clock event entries
  std::vector<AVRClockEventEntry *> clockEventPool;

  /*
    Function for adding a clock event
    @param callback: The callback function you'd like to run
    @param cycles: Number of cycles for the clock event
    @returns The callback function
  */
  AVRClockEventCallback addClockEvent(AVRClockEventCallback callback, int cycles);

  /*
    Function for updating a clock event
    @param callback: The callback function you'd like to run
    @param cycles: Number of cycles for the clock event
    @returns Whether update successful or not
  */
  bool updateClockEvent(AVRClockEventCallback callback, int cycles);

  /*
    Function for clearing a clock event
    @param callback: The callback function you'd like to run
    @returns Whether the clear was successful or not
  */
  bool clearClockEvent(AVRClockEventCallback callback);

  /* Function for the "ticking" of the CPU - checking clock events and interrupt statuses */
  void tick();

  /* For FALL 2024 Demo: A test function to fake an interrupt service routine + return */
  // void fakeISRAndRETI();
};