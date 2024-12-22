#include "../CPU/CPU.h"
#include "GPIO.h"
#include <unordered_map>

#pragma once

/* Timer 01 dividers  - last two are for external clock */
// int timer01Dividers[] = {0, 1, 8, 64, 256, 1024, 0, 0};
std::unordered_map<int, int> timer01Dividers = {
    {0, 0},
    {1, 1},
    {2, 8},
    {3, 64},
    {4, 256},
    {5, 1024},
    {6, 0}, // External clock - see ExternalClockMode
    {7, 0}, // Ditto
};

/* External clock module */
enum ExternalClockMode {
  FallingEdge = 6,
  RisingEdge = 7,
};

/* Array for generic timer dividers */
/***MIGHT need to switch to unordered_map ***/
// int timerDividers[8];
std::unordered_map<int, int> timerDividers;

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