/* Importing CPU and vector */
#include "../CPU/CPU.h"
#include <vector> 

#pragma once

struct AVRExternalInterrupt {
    // External Interrupt Control Register — Either EICRA or EICRB, 
    // based on which register holds the ISCx0/ISCx1 bits for this interrupt.
    u8 EICR;
    // External Interrupt Mask Register
    u8 EIMSK;
    // External Interrupt Flag Register
    u8 EIFR;
    // Offset of the ISCx0/ISCx1 bits in the EICRx register
    u8 ISCOffset; 
    // 0..7 — Bit index in the EIMSK / EIFR registers
    u8 index;
    // Index for the interrupt vector
    u8 interrupt;    
};

struct AVRPinChangeInterrupt {

    // Pin Change Interrupt Enable — bit index in PCICR/PCIFR
    u8 PCIE;
    // Pin Change Interrupt Control Register
    u8 PCICR;
    // Pin Change Interrupt Flag Register
    u8 PCIFR;
    // Pin Change Mask Register
    u8 PCMSK;

    // Pin change interrupt...address?
    u8 pinChangeInterrupt;
    u8 mask;
    u8 offset;
};

struct AVRPortConfig {

    // Register addresses
    // Port Input Pin
    u8 PIN;
    // Data Direction Register
    u8 DDR;
    // Data Register
    u8 PORT;
    // Settings for interrupt
    // pin change interrupt
    AVRPinChangeInterrupt *pinChange;
    // vector for external interrupts
    std::vector<AVRExternalInterrupt *> externalInterrupts;
};

// Specific configuration of the INT0 interrupt
struct INT0 : AVRExternalInterrupt {

    INT0(){
        this->EICR = 0x69;
        this->EIMSK = 0x3d;
        this->EIFR = 0x3c;
        this->index = 0;
        this->ISCOffset = 0;
        this->interrupt = 2;
    }
};

// Specific configuration of the INT1 interrupt
struct INT1 : AVRExternalInterrupt {

    INT1(){
        this->EICR = 0x69;
        this->EIMSK = 0x3d;
        this->EIFR = 0x3c;
        this->index = 1;
        this->ISCOffset = 2;
        this->interrupt = 4;  
    }
};

// Pin Change Interrupt 0 configuration
struct PCINT0 : AVRPinChangeInterrupt {

    PCINT0(){
        this->PCIE = 0;
        this->PCICR = 0x68;
        this->PCIFR = 0x3b;
        this->PCMSK = 0x6b;
        this->pinChangeInterrupt = 6;
        this->mask = 0xff;
        this->offset = 0;
    }
};

// Pin Change Interrupt 1 configuration
struct PCINT1 : AVRPinChangeInterrupt {
    PCINT1(){
        this->PCIE = 1;
        this->PCICR = 0x68;
        this->PCIFR = 0x3b;
        this->PCMSK = 0x6c;
        this->pinChangeInterrupt = 8;
        this->mask = 0xff;
        this->offset = 0;
    }
};

// Pin Change Interrupt 2 configuration
struct PCINT2 : AVRPinChangeInterrupt {
    PCINT2(){
        this->PCIE = 2;
        this->PCICR = 0x68;
        this->PCIFR = 0x3b;
        this->PCMSK = 0x6d;
        this->pinChangeInterrupt = 10;
        this->mask = 0xff;
        this->offset = 0;
    }
};

/* GPIO Listener "function type" */
typedef void (*GPIOListener) (u8 value, u8 oldValue);

/* External clock listener "function type" */
typedef void (*externalClockListener) (bool pinValue);


// configuring Port A
struct portAConfig : AVRPortConfig {

    portAConfig() {
        this->PIN = 0x20;
        this->DDR = 0x21;
        this->PORT = 0x22;
        // NO External interrupts
        this->externalInterrupts.clear(); 
    }
};

// configuring Port B
struct portBConfig : AVRPortConfig {

    portBConfig() {
        this->PIN = 0x23;
        this->DDR = 0x24;
        this->PORT = 0x25;
        // Not sure if this syntax works, but we are essentially instantiating a new PCINT0 for this
        this->pinChange = new PCINT0;
        // NO External interrupts
        this->externalInterrupts.clear(); 
    }
};

// Configuring Port C
struct portCConfig : AVRPortConfig {

    portCConfig() {
        this->PIN = 0x26;
        this->DDR = 0x27;
        this->PORT = 0x28;

        // 
        this->pinChange = new PCINT1;

        // No external interrupts
        this->externalInterrupts.clear();
    }
};

// Configuring Port D
struct portDConfig : AVRPortConfig {

    portDConfig() {
        this->PIN = 0x29;
        this->DDR = 0x2a;
        this->PORT = 0x2b;

        // Assuming PCINT2 is a class or type, instantiate it for pinChange
        this->pinChange = new PCINT2;

        // External interrupts: INT0, INT1 (skipping null values)
        this->externalInterrupts = { nullptr, nullptr, new INT0, new INT1 };
    }
};

// Configuring Port E
struct portEConfig : AVRPortConfig {

    portEConfig() {
        this->PIN = 0x2c;
        this->DDR = 0x2d;
        this->PORT = 0x2e;

        // No external interrupts
        this->externalInterrupts.clear();
    }
};

// Configuring Port F
struct portFConfig : AVRPortConfig {

    portFConfig() {
        this->PIN = 0x2f;
        this->DDR = 0x30;
        this->PORT = 0x31;

        // No external interrupts
        this->externalInterrupts.clear();
    }
};

// Configuring Port G
struct portGConfig : AVRPortConfig {

    portGConfig() {
        this->PIN = 0x32;
        this->DDR = 0x33;
        this->PORT = 0x34;

        // No external interrupts
        this->externalInterrupts.clear();
    }
};

/* Commenting the below out because they exceed the size of u8 (255) and I don't think we need em */

// Configuring Port H
// struct portHConfig : AVRPortConfig {

//     portHConfig() {
//         this->PIN = 0x100;
//         this->DDR = 0x101;
//         this->PORT = 0x102;

//         // No external interrupts
//         this->externalInterrupts.clear();
//     }
// };

// Configuring Port J
// struct portJConfig : AVRPortConfig {

//     portJConfig() {
//         this->PIN = 0x103;
//         this->DDR = 0x104;
//         this->PORT = 0x105;

//         // No external interrupts
//         this->externalInterrupts.clear();
//     }
// };

// Configuring Port K
// struct portKConfig : AVRPortConfig {

//     portKConfig() {
//         this->PIN = 0x106;
//         this->DDR = 0x107;
//         this->PORT = 0x108;

//         // No external interrupts
//         this->externalInterrupts.clear();
//     }
// };

// Configuring Port L
// struct portLConfig : AVRPortConfig {

//     portLConfig() {
//         this->PIN = 0x109;
//         this->DDR = 0x10a;
//         this->PORT = 0x10b;

//         // No external interrupts
//         this->externalInterrupts.clear();
//     }
// };

enum class PinState {
    Low,
    High,
    Input,
    InputPullUp
};

/* This allows timers to override specific GPIO pins */
enum class PinOverrideMode {
    None,
    Enable,
    Set,
    Clear,
    Toggle
};

enum class InterruptMode {
    LowLevel,
    Change,
    FallingEdge,
    RisingEdge
};

class AVRIOPort{

    public: 
    AVRIOPort(CPU *cpu, AVRPortConfig *porConfig);

    /* Vectors of our listeners for clock and GPIO */
    std::vector<externalClockListener> externalClockListeners;
    std::vector<GPIOListener> listeners;

    /* Vector of external interrupts */
    std::vector<AVRInterruptConfig *> externalInts;

    /* For tracking a pin change interrupt */
    AVRInterruptConfig *PCINT = nullptr;

    // Add comments for these...
    u8 pinValue = 0;
	u8 overrideMask = 0xff;
	u8 overrideValue = 0;
	u8 lastValue = 0;
	u8 lastDdr = 0;
	u8 lastPin = 0;
	u8 openCollector = 0;

    /*** Key Methods ***/

    /* Adding a GPIO listener */
    void addListener(GPIOListener listener);

    /* Removing a GPIO listener */
    void removeListener(GPIOListener listener);

    /**
    * Get the state of a given GPIO pin
    *
    * @param index Pin index to return from 0 to 7
    * @returns PinState.Low or PinState.High if the pin is set to output, PinState.Input if the pin is set
    *   to input, and PinState.InputPullUp if the pin is set to input and the internal pull-up resistor has
    *   been enabled.
    */
   /* Circle back AFTER constructor */
    PinState pinState(int index);

    /* Function for determining whether an interrupt should be triggered upon a changing pin state (switching from rising edge to falling edge or vice versa) */
    void toggleInterrupt(u8 pin, bool risingEdge);

};