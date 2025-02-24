/* The first part of the index: allll the includes */
#include <fstream>
#include <string>
// #include <unistd.h>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include "../CPU/CPU.h"
#include "../CPU/instruction.h"
#include "../CPU/interrupt.h"
#include "../peripherals/GPIO.h"
#include "../peripherals/timer.h"
#include "execute.h"
#include "compile.h"
#pragma once

// class AVRRunner;

/* The led in question */
extern bool led12;
extern bool led13;

extern AVRRunner *runner;

/* The hex string - for testing purposes */
extern std::string hexiCode;

void compileAndRun();

void executeProgram(std::string lilHexGal);

