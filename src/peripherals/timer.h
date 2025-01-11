#include "../CPU/CPU.h"
#include "GPIO.h"
#include <unordered_map>
#pragma once

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  Header for the Timer module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

// Declare instances for port configurations
extern portDConfig PDConfig;
extern portBConfig PBConfig;


// Timer 01 dividers  - last two are for external clock
extern std::unordered_map<int, int> timer01Dividers;

/* Enum for the external clock module */
enum class ExternalClockMode {
  FallingEdge = 6,
  RisingEdge = 7,
};

// Unordered map for generic timer dividers
extern std::unordered_map<int, int> timerDividers;

/* Configuration for a timer - parent struct */
struct AVRTimerConfig {
    // Timer configuration
    u8 bits; // Should be 8 or 16
    std::unordered_map<int, int> dividers = timerDividers;

    // Interrupt vectors
    u8 captureInterrupt;
    u8 compAInterrupt;
    u8 compBInterrupt;
    u8 compCInterrupt; // Optional, 0 = unused
    u8 ovfInterrupt;

    // Register addresses
    u8 TIFR;
    u8 OCRA;
    u8 OCRB;
    u8 OCRC; // Optional, 0 = unused
    u8 ICR;
    u8 TCNT;
    u8 TCCRA;
    u8 TCCRB;
    u8 TCCRC;
    u8 TIMSK;

    // TIFR bits
    u8 TOV;
    u8 OCFA;
    u8 OCFB;
    u8 OCFC; // Optional, if compCInterrupt != 0

    // TIMSK bits
    u8 TOIE;
    u8 OCIEA;
    u8 OCIEB;
    u8 OCIEC; // Optional, if compCInterrupt != 0

    // Output compare pins
    u16 compPortA;
    u8 compPinA;
    u16 compPortB;
    u8 compPinB;
    u16 compPortC; // Optional, 0 = unused
    u16 compPinC;

    // External clock pin (optional, 0 = unused)
    u16 externalClockPort;
    u8 externalClockPin;
};

/* Default timer configuration for Uno */
namespace DefaultTimerBits {
    constexpr u8 TOV = 1;
    constexpr u8 OCFA = 2;
    constexpr u8 OCFB = 4;
    constexpr u8 OCFC = 0; // Unused

    constexpr u8 TOIE = 1;
    constexpr u8 OCIEA = 2;
    constexpr u8 OCIEB = 4;
    constexpr u8 OCIEC = 0; // Unused
}

/* Timer 0 Configuration */
struct timer0Config: AVRTimerConfig {
    timer0Config() {
        bits = 8;
        captureInterrupt = 0; // Not available
        compAInterrupt = 0x1c;
        compBInterrupt = 0x1e;
        compCInterrupt = 0;
        ovfInterrupt = 0x20;
        TIFR = 0x35;
        OCRA = 0x47;
        OCRB = 0x48;
        OCRC = 0; // Not available
        ICR = 0;  // Not available
        TCNT = 0x46;
        TCCRA = 0x44;
        TCCRB = 0x45;
        TCCRC = 0; // Not available
        TIMSK = 0x6e;
        dividers = timer01Dividers;
        TOV = DefaultTimerBits::TOV;
        OCFA = DefaultTimerBits::OCFA;
        OCFB = DefaultTimerBits::OCFB;
        OCFC = DefaultTimerBits::OCFC;
        TOIE = DefaultTimerBits::TOIE;
        OCIEA = DefaultTimerBits::OCIEA;
        OCIEB = DefaultTimerBits::OCIEB;
        OCIEC = DefaultTimerBits::OCIEC;
        compPortA = PDConfig.PORT;
        compPinA = 6;
        compPortB = PDConfig.PORT;
        compPinB = 5;
        compPortC = 0; // Not available
        compPinC = 0;
        externalClockPort = PDConfig.PORT;
        externalClockPin = 4;
    }
};

/* Timer 1 Configuration */
struct timer1Config: AVRTimerConfig {
    timer1Config() {
        bits = 16;
        captureInterrupt = 0x14;
        compAInterrupt = 0x16;
        compBInterrupt = 0x18;
        compCInterrupt = 0;
        ovfInterrupt = 0x1a;
        TIFR = 0x36;
        OCRA = 0x88;
        OCRB = 0x8a;
        OCRC = 0; // Not available
        ICR = 0x86;
        TCNT = 0x84;
        TCCRA = 0x80;
        TCCRB = 0x81;
        TCCRC = 0x82;
        TIMSK = 0x6f;
        dividers = timer01Dividers;
        TOV = DefaultTimerBits::TOV;
        OCFA = DefaultTimerBits::OCFA;
        OCFB = DefaultTimerBits::OCFB;
        OCFC = DefaultTimerBits::OCFC;
        TOIE = DefaultTimerBits::TOIE;
        OCIEA = DefaultTimerBits::OCIEA;
        OCIEB = DefaultTimerBits::OCIEB;
        OCIEC = DefaultTimerBits::OCIEC;
        compPortA = PBConfig.PORT;
        compPinA = 1;
        compPortB = PBConfig.PORT;
        compPinB = 2;
        compPortC = 0; // Not available
        compPinC = 0;
        externalClockPort = PDConfig.PORT;
        externalClockPin = 5;
    }
};

/* Timer 2 Configuration */
struct timer2Config : AVRTimerConfig {
    timer2Config() {
        bits = 8;
        captureInterrupt = 0; // Not available
        compAInterrupt = 0x0e;
        compBInterrupt = 0x10;
        compCInterrupt = 0;
        ovfInterrupt = 0x12;
        TIFR = 0x37;
        OCRA = 0xb3;
        OCRB = 0xb4;
        OCRC = 0; // Not available
        ICR = 0;  // Not available
        TCNT = 0xb2;
        TCCRA = 0xb0;
        TCCRB = 0xb1;
        TCCRC = 0; // Not available
        TIMSK = 0x70;
        dividers = {
            {0, 0}, {1, 1}, {2, 8}, {3, 32}, {4, 64}, {5, 128}, {6, 256}, {7, 1024},
        };
        TOV = DefaultTimerBits::TOV;
        OCFA = DefaultTimerBits::OCFA;
        OCFB = DefaultTimerBits::OCFB;
        OCFC = DefaultTimerBits::OCFC;
        TOIE = DefaultTimerBits::TOIE;
        OCIEA = DefaultTimerBits::OCIEA;
        OCIEB = DefaultTimerBits::OCIEB;
        OCIEC = DefaultTimerBits::OCIEC;
        compPortA = PBConfig.PORT;
        compPinA = 3;
        compPortB = PDConfig.PORT;
        compPinB = 3;
        compPortC = 0; // Not available
        compPinC = 0;
        externalClockPort = 0; // Not available
        externalClockPin = 0;
    }
};

/* Enum for the timer mode */
enum class TimerMode {
  Normal,
  PWMPhaseCorrect,
  CTC,
  FastPWM,
  PWMPhaseFrequencyCorrect,
  Reserved
};

/* Enum for the update mode for the Timer/Counter Overflow */
enum class TOVUpdateMode {
  Max,
  Top,
  Bottom
};

/* Enum for the update mode for the Output Compare Registers */
enum class OCRUpdateMode {
  Immediate,
  Top,
  Bottom
};

// Special constants for Top Output Compare Registers A and Interrupt Control Register values
constexpr int TopOCRAVal = 1;
constexpr int TopICRVal = 2;

// Enable Toggle mode for OCxA in PWM Wave Generation mode
constexpr int OCToggle = 1;

/* TimerTopValue enum that acts like a union type--these are the values it can take on */
enum TimerTopValue {
    Value0xFF = 0xff,
    Value0x1FF = 0x1ff,
    Value0x3FF = 0x3ff,
    Value0xFFFF = 0xffff,
    TopOCRA = TopOCRAVal,
    TopICR = TopICRVal
};

/* Waveform Generation mode */
struct WGMConfig {
    TimerMode mode;           // Timer mode (e.g., Normal, FastPWM)
    TimerTopValue topValue;   // Top value (e.g., 0xff, TopOCRA)
    OCRUpdateMode ocrUpdate;  // OCR update mode
    TOVUpdateMode tovUpdate;  // TOV update mode
    int flags;              // WGM bits value
};

// Array of 8-bit WGM configurations
constexpr WGMConfig wgmModes8Bit[] = {
    //  {TimerMode, TimerTopValue, OCRUpdateMode, TOVUpdateMode, WGM Bits}
    {TimerMode::Normal, TimerTopValue::Value0xFF, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 0
    {TimerMode::PWMPhaseCorrect, TimerTopValue::Value0xFF, OCRUpdateMode::Top, TOVUpdateMode::Bottom, 0}, // 1
    {TimerMode::CTC, TimerTopValue::TopOCRA, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 2
    {TimerMode::FastPWM, TimerTopValue::Value0xFF, OCRUpdateMode::Bottom, TOVUpdateMode::Max, 0}, // 3
    {TimerMode::Reserved, TimerTopValue::Value0xFF, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 4
    {TimerMode::PWMPhaseCorrect, TimerTopValue::TopOCRA, OCRUpdateMode::Top, TOVUpdateMode::Bottom, OCToggle}, // 5
    {TimerMode::Reserved, TimerTopValue::Value0xFF, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 6
    {TimerMode::FastPWM, TimerTopValue::TopOCRA, OCRUpdateMode::Bottom, TOVUpdateMode::Top, OCToggle}, // 7
};

// Array of 16-bit WGM configurations
constexpr WGMConfig wgmModes16Bit[] = {
    //  {TimerMode, TimerTopValue, OCRUpdateMode, TOVUpdateMode, WGM Bits}
    {TimerMode::Normal, TimerTopValue::Value0xFFFF, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 0
    {TimerMode::PWMPhaseCorrect, TimerTopValue::Value0xFF, OCRUpdateMode::Top, TOVUpdateMode::Bottom, 0}, // 1
    {TimerMode::PWMPhaseCorrect, TimerTopValue::Value0x1FF, OCRUpdateMode::Top, TOVUpdateMode::Bottom, 0}, // 2
    {TimerMode::PWMPhaseCorrect, TimerTopValue::Value0x3FF, OCRUpdateMode::Top, TOVUpdateMode::Bottom, 0}, // 3
    {TimerMode::CTC, TimerTopValue::TopOCRA, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 4
    {TimerMode::FastPWM, TimerTopValue::Value0xFF, OCRUpdateMode::Bottom, TOVUpdateMode::Top, 0}, // 5
    {TimerMode::FastPWM, TimerTopValue::Value0x1FF, OCRUpdateMode::Bottom, TOVUpdateMode::Top, 0}, // 6
    {TimerMode::FastPWM, TimerTopValue::Value0x3FF, OCRUpdateMode::Bottom, TOVUpdateMode::Top, 0}, // 7
    {TimerMode::PWMPhaseFrequencyCorrect, TimerTopValue::TopICR, OCRUpdateMode::Bottom, TOVUpdateMode::Bottom, 0}, // 8
    {TimerMode::PWMPhaseFrequencyCorrect, TimerTopValue::TopOCRA, OCRUpdateMode::Bottom, TOVUpdateMode::Bottom, OCToggle}, // 9
    {TimerMode::PWMPhaseCorrect, TimerTopValue::TopICR, OCRUpdateMode::Top, TOVUpdateMode::Bottom, 0}, // 10
    {TimerMode::PWMPhaseCorrect, TimerTopValue::TopOCRA, OCRUpdateMode::Top, TOVUpdateMode::Bottom, OCToggle}, // 11
    {TimerMode::CTC, TimerTopValue::TopICR, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 12
    {TimerMode::Reserved, TimerTopValue::Value0xFFFF, OCRUpdateMode::Immediate, TOVUpdateMode::Max, 0}, // 13
    {TimerMode::FastPWM, TimerTopValue::TopICR, OCRUpdateMode::Bottom, TOVUpdateMode::Top, OCToggle}, // 14
    {TimerMode::FastPWM, TimerTopValue::TopOCRA, OCRUpdateMode::Bottom, TOVUpdateMode::Top, OCToggle}, // 15
};

/* Enum for possible values for compare bits */
enum CompBitsValue : int {
    Zero = 0,
    One = 1,
    Two = 2,
    Three = 3
};


/* Function for translating compare bits into pin override modes */
extern PinOverrideMode compToOverride(CompBitsValue comp);


// Force Output Compare (FOC) bits
constexpr int FOCA = 1 << 7;
constexpr int FOCB = 1 << 6;
constexpr int FOCC = 1 << 5;

/* The AVRTimer class!!!! */
class AVRTimer {
    public:

    // The main clock event callback function
    AVRClockEventCallback mainClockEvent;
    const u16 MAX; // Calculated based on the config's bit size (16-bit or 8-bit)
    u64 lastCycle = 0; // Track the last CPU cycle
    u16 ocrA = 0; // Output Compare Register A
    u16 nextOcrA = 0; // Next OCR A value
    u16 ocrB = 0; // Output Compare Register B
    u16 nextOcrB = 0; // Next OCR B value
    bool hasOCRC; // Whether OCR C is available
    u16 ocrC = 0; // Output Compare Register C
    u16 nextOcrC = 0; // Next OCR C value
    OCRUpdateMode ocrUpdateMode = OCRUpdateMode::Immediate; // OCR update mode
    TOVUpdateMode tovUpdateMode = TOVUpdateMode::Max; // Timer Overflow (TOV) update mode
    u16 icr = 0; // Input Capture Register (only for 16-bit timers)
    TimerMode timerMode; // Current timer mode
    TimerTopValue topValue; // Top value for the timer
    u16 tcnt = 0; // Timer/Counter register
    u16 tcntNext = 0; // Next Timer/Counter value
    CompBitsValue compA; // Compare bits for channel A
    CompBitsValue compB; // Compare bits for channel B
    CompBitsValue compC; // Compare bits for channel C
    bool tcntUpdated = false; // Whether TCNT was updated
    bool updateDivider = false; // Whether the divider needs updating
    bool countingUp = true; // Whether the timer is counting up
    int divider = 0; // Timer clock divider
    AVRIOPort* externalClockPort = nullptr; // External clock port
    bool externalClockRisingEdge = false; // External clock mode: rising edge
    u8 highByteTemp = 0; // Temporary high-byte register for 16-bit access (section 16.3 of the datasheet)

    // For tracking the mainCPU and config
    CPU *mainCPU;
    AVRTimerConfig *mainConfig;

    /* Overflow interrupt */
    struct OVFInterrupt: AVRInterruptConfig {
        OVFInterrupt(const AVRTimerConfig *config) {
            address = config->ovfInterrupt;
            flagRegister = config->TIFR;
            flagMask = config->TOV;
            enableRegister = config->TIMSK;
            enableMask = config->TOIE;
        }
    };
    /* Output Compare Flag A interrupt */
    struct OCFAInterrupt : public AVRInterruptConfig {
        OCFAInterrupt(const AVRTimerConfig *config) {
            address = config->compAInterrupt;
            flagRegister = config->TIFR;
            flagMask = config->OCFA;
            enableRegister = config->TIMSK;
            enableMask = config->OCIEA;
        }
    };
    /* Output Compare Flag B interrupt */
    struct OCFBInterrupt : public AVRInterruptConfig {
        OCFBInterrupt(const AVRTimerConfig *config) {
            address = config->compBInterrupt;
            flagRegister = config->TIFR;
            flagMask = config->OCFB;
            enableRegister = config->TIMSK;
            enableMask = config->OCIEB;
        }
    };
    /* Output Compare Flag C interrupt */
    struct OCFCInterrupt : public AVRInterruptConfig {
        OCFCInterrupt(const AVRTimerConfig *config) {
            address = config->compCInterrupt;
            flagRegister = config->TIFR;
            flagMask = config->OCFC;
            enableRegister = config->TIMSK;
            enableMask = config->OCIEC;
        }
    };

    /* One of each interrupt for use */
    OCFAInterrupt *defaultOCFAInterrupt;
    OCFBInterrupt *defaultOCFBInterrupt;
    OCFCInterrupt *defaultOCFCInterrupt;
    OVFInterrupt *defaultOVFInterrupt;

    /* System reset! */
    void reset();

    /* Getter for TCCRA */
    u8 getTCCRA();

    /* Getter for TCCRB */
    u8 getTCCRB();

    /* Getter for TIMSK */
    u8 getTIMSK();

    /* Get the Clock Select (CS) bits */
    u8 getCS();

    /* Get the Waveform Generation Mode (WGM) bits */
    u8 getWGM();

    /* Get the TOP value for the timer */
    u16 getTOP();

    /* Get the OCR mask for the timer */
    u16 getOCRMask();

    /* Expose the raw value of TCNT for debugging */
    u16 getDebugTCNT();

    /* 
        For updating the Waveform Generation Mode (WGM)
        @returns No return value
    */
    void updateWGMConfig();

    /* 
        For counting!
        @param reschedule: Whether we should reschedule the clock event
        @param external: Used for computing the counter delta
        @returns No return value 
    */
    void count(bool reschedule = true, bool external = false);

    /* 
        For managing external clock callbacks 
        @param value: Checked against whether the external clock has rising edge enabled
        @returns No return value 
    */
    void externalClockCallback(bool value);

    /* 
        For incrementing/decrementing the value when in phase PWM mode
        @param value: Current timer value
        @param delta: Difference between where we are and where we wanna be
        @returns  The new value & MAX
    */
    u16 phasePwmCount(u16 value, u8 delta);

    /* 
        For setting interrupt flags when different compare thresholds are reached 
        @param value: Current timer value
        @param prevValue: The previous timer value
    */
    void timerUpdated(u16 value, u16 prevValue);

    /* 
        Checking for force compare and updating if needed 
        @param value: Value to check against FOCA, FOCB, and FOCC
        @returns No return value
    */
    void checkForceCompare(u8 value);

    /* 
        For updating a compare pin
        @param compValue: The CompBitsValue of interest
        @param pinName: A character representing the pin name (e.g., A or B)
        @param bottom: Whether the count is at the bottom value
        @returns No return value
    */
    void updateCompPin(CompBitsValue compValue, char pinName, bool bottom = false);

    /*
        For updating compare pin A
        @param value: The pin override mode
        @returns No return value 
    */
    void updateCompA(PinOverrideMode value);

    /*
        For updating compare pin B 
        @param value: The pin override mode
        @returns No return value
    */
    void updateCompB(PinOverrideMode value);

    /* 
        For updating compare pin C 
        @param value: The pin override mode
        @returns No return value
    */
    void updateCompC(PinOverrideMode value);

    /* 
        Constructor!
        @param cpu: Pointer to the CPU of interest
        @param config: The AVRTimerConfig we want to use  
    */
    AVRTimer(CPU *cpu, AVRTimerConfig *config);
};