#include "timer.h"

// Define the instances
portDConfig PDConfig;
portBConfig PBConfig;

AVRTimer::AVRTimer(CPU *cpu, AVRTimerConfig *config) : MAX(config->bits == 16 ? 0xffff : 0xff),
      hasOCRC(config->OCRC > 0) 
{
    this->mainCPU = cpu;
    this->mainConfig = config;

    // Setting up the main interrupts
  this->defaultOCFAInterrupt = new OCFAInterrupt(this->mainConfig);
  this->defaultOCFBInterrupt = new OCFBInterrupt(this->mainConfig);
  this->defaultOCFCInterrupt = new OCFCInterrupt(this->mainConfig);
  this->defaultOVFInterrupt = new OVFInterrupt(this->mainConfig);


  this->updateWGMConfig();
}

void AVRTimer::reset(){
    this->divider = 0;
    this->lastCycle = 0;
    this->ocrA = 0;
    this->nextOcrA = 0;
    this->ocrB = 0;
    this->nextOcrB = 0;
    this->ocrC = 0;
    this->nextOcrC = 0;
    this->icr = 0;
    this->tcnt = 0;
    this->tcntNext = 0;
    this->tcntUpdated = false;
    this->countingUp = false;
    this->updateDivider = true;
}

u8 AVRTimer::getTCCRA(){
    return this->mainCPU->data[this->mainConfig->TCCRA];
}

u8 AVRTimer::getTCCRB(){
    return this->mainCPU->data[this->mainConfig->TCCRB];
}

u8 AVRTimer::getTIMSK(){
    return this->mainCPU->data[this->mainConfig->TIMSK];
}

u8 AVRTimer::getCS() {
    return this->getTCCRB() & 0x7; // Extract the lowest 3 bits
}

u8 AVRTimer::getWGM() {
    const u8 mask = (this->mainConfig->bits == 16) ? 0x18 : 0x8;
    return ((this->getTCCRB() & mask) >> 1) | (this->getTCCRA() & 0x3);
}

u16 AVRTimer::getTOP() {
    switch (this->topValue) {
        case TimerTopValue::TopOCRA:
            return this->ocrA;
        case TimerTopValue::TopICR:
            return this->icr;
        default:
            return this->topValue;
    }
}

u16 AVRTimer::getOCRMask() {
    switch (this->topValue) {
        case TimerTopValue::TopOCRA:
        case TimerTopValue::TopICR:
            return 0xffff;
        default:
            return static_cast<u16>(this->topValue);
    }
}

u16 AVRTimer::getDebugTCNT() {
    return this->tcnt;
}

/* Tested: ✅ */
void AVRTimer::updateWGMConfig() {

    // Extracting helpful information about the current mode and TCCRA
    const AVRTimerConfig *config = this->mainConfig;
    const auto &wgmModes = (config->bits == 16) ? wgmModes16Bit : wgmModes8Bit;
    std::cout << "Is the config 8 bits? It should be... " << (wgmModes == wgmModes8Bit) << std::endl;
    const u8 TCCRA = this->mainCPU->data[config->TCCRA];
    std::cout << "Data at TCCRA should be 255 " << int(TCCRA) << std::endl;
    const WGMConfig &wgmConfig = wgmModes[this->getWGM()];
    std::cout << "getWGM for testing should be 3: " << int(this->getWGM()) << "And wgmModes should have top value of 255 and OCRUpdateMode 2: " << int(wgmConfig.topValue) << " and " << int(wgmConfig.ocrUpdate) <<  std::endl;

    this->timerMode = wgmConfig.mode;
    this->topValue = wgmConfig.topValue;
    this->ocrUpdateMode = wgmConfig.ocrUpdate;
    this->tovUpdateMode = wgmConfig.tovUpdate;

    // Getting the PWM Mode
    bool pwmMode = (this->timerMode == TimerMode::FastPWM ||
                this->timerMode == TimerMode::PWMPhaseCorrect ||
                this->timerMode == TimerMode::PWMPhaseFrequencyCorrect);

    std::cout << "mode should be true: " << pwmMode << std::endl;

    // Update compA
    u8 prevCompA = this->compA;
    std::cout << "prevCompA " << int(prevCompA) << std::endl;
    this->compA = CompBitsValue((TCCRA >> 6) & 0x3);
    std::cout << "current CompA " << int(this->compA) << std::endl;
    if (this->compA == 1 && pwmMode && !(wgmConfig.flags & OCToggle)) {
        this->compA = CompBitsValue(0);
    }
    if ((prevCompA != 0) != (this->compA != 0)) {
        this->updateCompA(this->compA ? PinOverrideMode::Enable : PinOverrideMode::None);
    }

    // Update compB
    u8 prevCompB = this->compB;
    std::cout << "prevCompB " << int(prevCompB) << std::endl;
    this->compB = CompBitsValue((TCCRA >> 4) & 0x3);
    std::cout << "current CompB " << int(this->compB) << std::endl;
    if (this->compB == 1 && pwmMode) {
        this->compB = CompBitsValue(0); // Reserved, according to the datasheet
    }
    if ((prevCompB != 0) != (this->compB != 0)) {
        this->updateCompB(this->compB ? PinOverrideMode::Enable : PinOverrideMode::None);
    }

    // Update compC if available
    if (this->hasOCRC) {
        u8 prevCompC = this->compC;
        this->compC = CompBitsValue((TCCRA >> 2) & 0x3);
        if (this->compC == 1 && pwmMode) {
            this->compC = CompBitsValue(0); // Reserved, according to the datasheet
        }
        if ((prevCompC != 0) != (this->compC != 0)) {
            this->updateCompC(this->compC ? PinOverrideMode::Enable : PinOverrideMode::None);
        }
    }

}

/* Tested: ✅ */
u16 AVRTimer::phasePwmCount(u16 value, u8 delta) {

   // Extracting relevant constants
    const u16 ocrA = this->ocrA;
    const u16 ocrB = this->ocrB;
    const u16 ocrC = this->ocrC;
    const bool hasOCRC = this->hasOCRC;
    const u16 TOP = this->getTOP(); // I think this is right, circle back if issues...
    const u16 MAX = this->MAX;
    const bool tcntUpdated = this->tcntUpdated;

    if (value == 0 && TOP == 0) {
        std::cout << "Value and top are both zero" << std::endl;
        delta = 0;
        if (this->ocrUpdateMode == OCRUpdateMode::Top) {
            this->ocrA = this->nextOcrA;
            this->ocrB = this->nextOcrB;
            this->ocrC = this->nextOcrC;
        }
    }

    while (delta > 0) {
        // If countingUp is true, increment value
        if (this->countingUp) {
            std::cout << "Counting upppp" << std::endl;
            value++;
            // If we've reached the top value and tcnt isn't updated, update OCR mode and move it along
            if (value == TOP && !tcntUpdated) {
                std::cout << "We reached the top, counting up now false" << std::endl;
                this->countingUp = false;
                if (this->ocrUpdateMode == OCRUpdateMode::Top) {
                    std::cout << "OCR update mode is top" << std::endl;
                    this->ocrA = this->nextOcrA;
                    this->ocrB = this->nextOcrB;
                    this->ocrC = this->nextOcrC;
                }
            }
        } else { // Otherwise, decrement value
            std::cout << "Counting downnn" << std::endl;
            value--;
            // If we've reached the bottom value and tcnt isn't updated, update OCR mode and move it along
            if (value == 0 && !tcntUpdated) {
                std::cout << "We reached the bottom, counting up now true" << std::endl;
                this->countingUp = true;
                this->mainCPU->setInterruptFlag(this->defaultOVFInterrupt);
                if (this->ocrUpdateMode == OCRUpdateMode::Bottom) {
                    std::cout << "OCR update mode is bottom" << std::endl;
                    this->ocrA = this->nextOcrA;
                    this->ocrB = this->nextOcrB;
                    this->ocrC = this->nextOcrC;
                }
            }
        }
        // If we haven't updated tcnt
        if (!tcntUpdated) {

            if (value == ocrA) {
                this->mainCPU->setInterruptFlag(this->defaultOCFAInterrupt);
                if (this->compA) {
                    this->updateCompPin(this->compA, 'A');
                }
            }
            if (value == ocrB) {
                this->mainCPU->setInterruptFlag(this->defaultOCFBInterrupt);
                if (this->compB) {
                    this->updateCompPin(this->compB, 'B');
                }
            }
            if (hasOCRC && value == ocrC) {
                this->mainCPU->setInterruptFlag(this->defaultOCFCInterrupt);
                if (this->compC) {
                    this->updateCompPin(this->compC, 'C');
                }
            }
        }
        delta--;
    }
    return value & MAX;
}

/* Tested: ✅ */
void AVRTimer::timerUpdated(u16 value, u16 prevValue) {
    // Extracting relevant constants
    const u16 ocrA = this->ocrA;
    const u16 ocrB = this->ocrB;
    const u16 ocrC = this->ocrC;
    const bool hasOCRC = this->hasOCRC;

    // Detecting overflow
    bool overflow = (prevValue > value);
    if(overflow){
        std::cout << "Overflow has occurred!" << std::endl;
    }

    // Handle OCR A
    if (((prevValue < ocrA || overflow) && value >= ocrA) || (prevValue < ocrA && overflow)) {
        std::cout << "Handling OCR A" << std::endl;
        this->mainCPU->setInterruptFlag(this->defaultOCFAInterrupt);
        if (this->compA) {
            this->updateCompPin(this->compA, 'A');
        }
    }

    // Handle OCR B
    if (((prevValue < ocrB || overflow) && value >= ocrB) || (prevValue < ocrB && overflow)) {
        std::cout << "Handling OCR B" << std::endl;
        this->mainCPU->setInterruptFlag(this->defaultOCFBInterrupt);
        if (this->compB) {
            this->updateCompPin(this->compB, 'B');
        }
    }

    // Handle OCR C if available
    if (hasOCRC && (((prevValue < ocrC || overflow) && value >= ocrC) || (prevValue < ocrC && overflow))) {
        std::cout << "Handling OCR C" << std::endl;
        this->mainCPU->setInterruptFlag(this->defaultOCFCInterrupt);
        if (this->compC) {
            this->updateCompPin(this->compC, 'C');
        }
    }
}

int main(){

    /* testing phasePWMCount() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // Value and TOP are 0
    // std::cout << "TOP value is: " << int(timer->getTOP()) << std::endl;
    // N/A, TOP is always 255 in our case
    // delta = 10, countingUp true, value == TOP - 5, and tcntUpdated is false
    // timer->countingUp = true;
    // timer->tcntUpdated = false;
    // timer->ocrA = 250;
    // timer->ocrB = 250;
    // timer->compA = CompBitsValue(1);
    // timer->compB = CompBitsValue(1);
    // timer->ocrUpdateMode = OCRUpdateMode::Top;
    // int result = timer->phasePwmCount((timer->getTOP() - 5), 10);
    // std::cout << "The result is: " << result << std::endl;
    // delta = 10, countingUp false, value == 5, and tcntUpdated is false
    // timer->countingUp = false;
    // timer->tcntUpdated = false;
    // timer->ocrA = 45;
    // timer->ocrB = 45;
    // timer->compA = CompBitsValue(1);
    // timer->compB = CompBitsValue(1);
    // timer->ocrUpdateMode = OCRUpdateMode::Bottom;
    // int result = timer->phasePwmCount(5, 10);
    // std::cout << "The result is: " << result << std::endl;

    /* testing timerUpdated() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // // Set OCR a and OCR B
    // timer->ocrA = 20;
    // timer->ocrB = 20;
    // timer->compA = CompBitsValue(1);
    // timer->compB = CompBitsValue(1);
    // timer->timerUpdated(21,19);

    /* testing check checkForceCompare() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // timer->timerMode = TimerMode::Normal;
    // u8 testVal = 0b00000000;
    // timer->checkForceCompare(testVal);

    /* Testing updateWGMConfig() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // // Setting dummy value for TCCRA
    // cpu->data[timer->mainConfig->TCCRA] = 0b11111101;
    // // Confirm that the values initial values are correct...
    // timer->updateWGMConfig();

    /* Testing updateCompPin() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // // Try different timer modes
    // timer->timerMode = TimerMode::Reserved;
    // timer->updateCompPin(CompBitsValue::One, 'B', true);

    /* Testing updateCompB() and C() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // cpu->data[testIO->mainPortConfig->DDR] = 0b11111111;
    // cpu->data[testIO->mainPortConfig->PORT] = 0b11111111;
    // timer->updateCompB(PinOverrideMode::None);
    // timer->updateCompC(PinOverrideMode::None);

    /* Testing updateCompA() ✅ */
    // std::vector<u16> testPM(1024);
    // CPU *cpu = new CPU(testPM);
    // portDConfig *PD = new portDConfig;
    // AVRIOPort *testIO = new AVRIOPort(cpu, PD);
    // timer0Config *tzero = new timer0Config;
    // AVRTimer *timer = new AVRTimer(cpu, tzero);
    // cpu->data[testIO->mainPortConfig->DDR] = 0b11111111;
    // cpu->data[testIO->mainPortConfig->PORT] = 0b11111111;
    // // Start by just printing out the values...
    // timer->updateCompA(PinOverrideMode::Enable);

}

/* Tested: ✅ */
void AVRTimer::checkForceCompare(u8 value) {
    // Return if the timer is in a PWM mode
    if (this->timerMode == TimerMode::FastPWM ||
        this->timerMode == TimerMode::PWMPhaseCorrect ||
        this->timerMode == TimerMode::PWMPhaseFrequencyCorrect) {
        std::cout << "We got a PWM mode, abort!" << std::endl;
        // FOCnA/FOCnB/FOCnC bits are only active when the WGMn3:0 bits specify a non-PWM mode
        return;
    }

    // Check and update compare pins for A, B, C
    if (value & FOCA) {
        std::cout << "FOCA test passed!" << std::endl;
        this->updateCompPin(this->compA, 'A');
    }
    if (value & FOCB) {
        std::cout << "FOCB test passed!" << std::endl;
        this->updateCompPin(this->compB, 'B');
    }
    if (this->mainConfig->compPortC && (value & FOCC)) {
        std::cout << "FOCC test should not be passed!" << std::endl;
        this->updateCompPin(this->compC, 'C');
    }
}

/* Tested: ✅ */
void AVRTimer::updateCompPin(CompBitsValue compValue, char pinName, bool bottom) {
    // Setting up new value and determining if it's inverting mode and if it's set
    PinOverrideMode newValue = PinOverrideMode::None;
    bool invertingMode = (int(compValue) == 3);
    std::cout << "invertingMode is " << invertingMode << std::endl;
    bool isSet = (this->countingUp == invertingMode);
    // Update the newValue based on the timer mode and compValue
    switch (this->timerMode) {
        case TimerMode::Normal:
        case TimerMode::CTC:
            std::cout << "Mode is Normal or CTC" << std::endl;
            newValue = compToOverride(compValue);
            std::cout << "newValue: " << int(newValue) << std::endl;
            break;

        case TimerMode::FastPWM:
            std::cout << "Mode is FastPWM" << std::endl;
            if (int(compValue) == 1) {
                std::cout << "Compvalue is 1" << std::endl;
                newValue = bottom ? PinOverrideMode::None : PinOverrideMode::Toggle;
                std::cout << "newValue: " << int(newValue) << std::endl;
            } else {
                newValue = (invertingMode != bottom) ? PinOverrideMode::Set : PinOverrideMode::Clear;
                std::cout << "newValue: " << int(newValue) << std::endl;
            }
            break;

        case TimerMode::PWMPhaseCorrect:
        case TimerMode::PWMPhaseFrequencyCorrect:
            std::cout << "Mode is PWMPhaseCorrect or PWMPhaseFrequencyCorrect" << std::endl;
            if (int(compValue) == 1) {
                std::cout << "Compvalue is 1" << std::endl;
                newValue = PinOverrideMode::Toggle;
                std::cout << "newValue: " << int(newValue) << std::endl;
            } else {
                newValue = isSet ? PinOverrideMode::Set : PinOverrideMode::Clear;
                std::cout << "newValue: " << int(newValue) << std::endl;
            }
            break;
        case TimerMode::Reserved:
        default:
            break;
    }
    // Finally, if the pinoverride mode isn't none, then update the appropriate compare pin
    if (newValue != PinOverrideMode::None) {
            if (pinName == 'A') {
                this->updateCompA(newValue);
            } else if (pinName == 'B') {
                this->updateCompB(newValue);
            } else if (pinName == 'C') {
                this->updateCompC(newValue);
            }
    }
}

/* Tested: ✅ */
void AVRTimer::updateCompA(PinOverrideMode value){

    // Retrieve the port and pin configuration
    u16 compPortA = this->mainConfig->compPortA;
    std::cout << "compPortA: " << int(compPortA) << std::endl;
    u8 compPinA = this->mainConfig->compPinA;
    std::cout << "compPinA: " << int(compPinA) << std::endl;

    // Get the corresponding GPIO port from the CPU
    AVRIOPort *port = this->mainCPU->GPIOByPort[compPortA];

    // If the port exists, call the timerOverridePin function
    if (port) {
        std::cout << "port exists. calling timerOverridePin" << std::endl;
        port->timerOverridePin(compPinA, value);
    }
}

/* Tested: ✅ */
void AVRTimer::updateCompB(PinOverrideMode value) {
    // Retrieve the port and pin configuration
    u16 compPortB = this->mainConfig->compPortB;
    std::cout << "compPortB: " << int(compPortB) << std::endl;
    u8 compPinB = this->mainConfig->compPinB;
    std::cout << "compPinB: " << int(compPinB) << std::endl;

    // Get the corresponding GPIO port from the CPU
    AVRIOPort *port = this->mainCPU->GPIOByPort[compPortB];

    // If the port exists, call the timerOverridePin function
    if (port) {
        std::cout << "port exists. calling timerOverridePin" << std::endl;
        port->timerOverridePin(compPinB, value);
    }
}

/* Tested: ✅ */
void AVRTimer::updateCompC(PinOverrideMode value) {
    // Retrieve the port and pin configuration
    u16 compPortC = this->mainConfig->compPortC;
    std::cout << "compPortC: " << int(compPortC) << std::endl;
    u8 compPinC = this->mainConfig->compPinC;
    std::cout << "compPinC: " << int(compPinC) << std::endl;

    // Get the corresponding GPIO port from the CPU
    AVRIOPort *port = this->mainCPU->GPIOByPort[compPortC];

    // If the port exists, call the timerOverridePin function
    if (port) {
        std::cout << "port exists. calling timerOverridePin" << std::endl;
        port->timerOverridePin(compPinC, value);
    }
}

