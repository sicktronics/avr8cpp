/* The first part of the index: allll the includes */

#include "../CPU/CPU.h"
#include "../CPU/instruction.h"
#include "../CPU/interrupt.h"
#include "../peripherals/GPIO.h"
#include "../peripherals/timer.h"
#include "compile.h"
#pragma once

// class AVRRunner;

/* The led in question */
extern bool led12;
extern bool led13;

/* The hex string - for testing purposes */
extern std::string hexiCode;

void compileAndRun();

void executeProgram(std::string lilHexGal);

/* for testing only */
void appendBoolToFile(const std::string& filePath, bool value);