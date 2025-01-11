#include "timer.h"

/*
  This project is a translation of Uri Shaked's avr8js repository: 
    https://github.com/wokwi/avr8js
  The avr8js repository is part of Wokwi. Check out the awesome work they're doing here: 
    https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog

  CPP for the Timer module.

  - Translated into C++ by:  Parker Caywood Mayer
  - Last modified:           Jan 2025
*/

// Initializing port configs
portDConfig PDConfig;
portBConfig PBConfig;

PinOverrideMode compToOverride(CompBitsValue comp) {
    switch (comp) {
        case CompBitsValue::One:
            return PinOverrideMode::Toggle;
        case CompBitsValue::Two:
            return PinOverrideMode::Clear;
        case CompBitsValue::Three:
            return PinOverrideMode::Set;
        default:
            return PinOverrideMode::Enable;
    }
}

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

std::unordered_map<int, int> timerDividers = {
    {0, 0},
    {1, 1},
    {2, 8},
    {3, 64},
    {4, 256},
    {5, 1024},
    {6, 0}, // External clock - see ExternalClockMode
    {7, 0}, // Ditto
};

AVRTimer::AVRTimer(CPU *cpu, AVRTimerConfig *config) : MAX(config->bits == 16 ? 0xffff : 0xff),
      hasOCRC(config->OCRC > 0) 
{
    // Setting up the mainCPU and mainConfig
    this->mainCPU = cpu;
    this->mainConfig = config;

    // Setting up the count clock event
    this->mainClockEvent = std::make_shared<std::function<void()>>([this]() { this->count(); });

    // Size the timer dividers

    // Setting up the main interrupts
    this->defaultOCFAInterrupt = new OCFAInterrupt(this->mainConfig);
    this->defaultOCFBInterrupt = new OCFBInterrupt(this->mainConfig);
    this->defaultOCFCInterrupt = hasOCRC ? new OCFCInterrupt(config) : nullptr;
    this->defaultOVFInterrupt = new OVFInterrupt(this->mainConfig);

    // Update Waveform Generation Mode (WGM) configuration
    this->updateWGMConfig();

    // Setup read hook for TCNT
    auto readTCNTHook = std::make_shared<std::function<u8(u16)>>([this](u16 address) -> u8 {
        this->count(false);
        // Shift over by 8 bits
        if (this->mainConfig->bits == 16) {
            this->mainCPU->data[address + 1] = this->tcnt >> 8;
        }
        return (this->mainCPU->data[address] = this->tcnt & 0xff);
    });
    this->mainCPU->readHookFunctions[config->TCNT] = readTCNTHook;

    // Set up write hook for TCNT
    this->mainCPU->writeHookVector[config->TCNT] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this](u8 value, u8, u16, u8) -> bool {
            this->tcntNext = (this->highByteTemp << 8) | value;
            this->countingUp = true;
            this->tcntUpdated = true;
            this->mainCPU->updateClockEvent(this->mainClockEvent, 0);
            if (this->divider) {
                this->timerUpdated(this->tcntNext, this->tcntNext);
            }
            return true;
        });
    // Set up write hook for OCRA
    this->mainCPU->writeHookVector[config->OCRA] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this](u8 value, u8, u16, u8) -> bool {
            // std::cout<< "Inside write OCRA hook" << std::endl;
            this->nextOcrA = (this->highByteTemp << 8) | value;
            if (this->ocrUpdateMode == OCRUpdateMode::Immediate) {
                this->ocrA = this->nextOcrA;
            }
            return true;
        });
    // Set up write hook for OCRB
    this->mainCPU->writeHookVector[config->OCRB] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this](u8 value, u8, u16, u8) -> bool {
            this->nextOcrB = (this->highByteTemp << 8) | value;
            if (this->ocrUpdateMode == OCRUpdateMode::Immediate) {
                this->ocrB = this->nextOcrB;
            }
            return true;
        });
    // Set up write hook for OCRC, if relevant
    if (hasOCRC) {
        this->mainCPU->writeHookVector[config->OCRC] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
            [this](u8 value, u8, u16, u8) -> bool {
                this->nextOcrC = (this->highByteTemp << 8) | value;
                if (this->ocrUpdateMode == OCRUpdateMode::Immediate) {
                    this->ocrC = this->nextOcrC;
                }
                return true;
            });
    }    
    
    // 16-bit configuration
    if (config->bits == 16) {
        this->mainCPU->writeHookVector[config->ICR] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
            [this](u8 value, u8, u16, u8) -> bool {
                this->icr = (this->highByteTemp << 8) | value;
                return true;
            });

        auto updateTempRegister = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
            [this](u8 value, u8, u16, u8) -> bool {
                this->highByteTemp = value;
                return true;
            });

        auto updateOCRHighRegister = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
            [this](u8 value, u8, u16 address, u8) -> bool {
                this->highByteTemp = value & (this->getOCRMask() >> 8);
                this->mainCPU->data[address] = this->highByteTemp;
                return true;
            });

        this->mainCPU->writeHookVector[config->TCNT + 1] = updateTempRegister;
        this->mainCPU->writeHookVector[config->OCRA + 1] = updateOCRHighRegister;
        this->mainCPU->writeHookVector[config->OCRB + 1] = updateOCRHighRegister;
        if (hasOCRC) {
            this->mainCPU->writeHookVector[config->OCRC + 1] = updateOCRHighRegister;
        }
        this->mainCPU->writeHookVector[config->ICR + 1] = updateTempRegister;
    }

    // Set up write hook for TCCRA
    this->mainCPU->writeHookVector[config->TCCRA] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this, config](u8 value, u8, u16, u8) -> bool {
            this->mainCPU->data[config->TCCRA] = value;
            this->updateWGMConfig();
            return true;
    });

    // Set up write hook for TCCRB
    this->mainCPU->writeHookVector[config->TCCRB] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this, config](u8 value, u8, u16, u8) -> bool {
            if (!config->TCCRC) {
                this->checkForceCompare(value);
                value &= ~(FOCA | FOCB);
            }
            this->mainCPU->data[config->TCCRB] = value;
            this->updateDivider = true;
            this->mainCPU->clearClockEvent(this->mainClockEvent);
            this->mainCPU->addClockEvent(this->mainClockEvent, 0);
            this->updateWGMConfig();
            return true;
        });

    if (config->TCCRC) {
        this->mainCPU->writeHookVector[config->TCCRC] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
            [this](u8 value, u8, u16, u8) -> bool {
                this->checkForceCompare(value);
                return true;
            });
    }

    this->mainCPU->writeHookVector[config->TIFR] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this, config](u8 value, u8, u16, u8) -> bool {
            this->mainCPU->data[config->TIFR] = value;
            this->mainCPU->clearInterruptByFlag(this->defaultOVFInterrupt, value);
            this->mainCPU->clearInterruptByFlag(this->defaultOCFAInterrupt, value);
            this->mainCPU->clearInterruptByFlag(this->defaultOCFBInterrupt, value);
            return true;
        });

    this->mainCPU->writeHookVector[config->TIMSK] = std::make_shared<std::function<bool(u8, u8, u16, u8)>>(
        [this](u8 value, u8, u16, u8) -> bool {
            this->mainCPU->updateInterruptsEnabled(this->defaultOVFInterrupt, value);
            this->mainCPU->updateInterruptsEnabled(this->defaultOCFAInterrupt, value);
            this->mainCPU->updateInterruptsEnabled(this->defaultOCFBInterrupt, value);
            return true;
        });
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

void AVRTimer::count(bool reschedule, bool external) {

   const u32 delta = this->mainCPU->cycles - this->lastCycle;
   
    if ((this->divider && delta >= this->divider) || external) {

        const u16 counterDelta = external ? 1 : delta / this->divider; 

        this->lastCycle += counterDelta * this->divider;
        const u16 val = this->tcnt;

        const TimerMode timerMode = this->timerMode;
        const u16 TOP = this->getTOP(); // Using this const wherever the const in ts is used

        const bool phasePwm = (timerMode == TimerMode::PWMPhaseCorrect || timerMode == TimerMode::PWMPhaseFrequencyCorrect);

        const u16 newVal = phasePwm
            ? this->phasePwmCount(val, counterDelta)
            : (val + counterDelta) % (TOP + 1);
            
        const bool overflow = (val + counterDelta > TOP);

        // Handle TCNT updates - A CPU write has priority over all counter clear or count operations.
        if (!this->tcntUpdated) {
            this->tcnt = newVal;
            if (!phasePwm) {
                this->timerUpdated(newVal, val);
            }
        }

        // Handle FastPWM and overflow scenarios
        if (!phasePwm) {
            if (timerMode == TimerMode::FastPWM && overflow) {
                if (this->compA) {
                    this->updateCompPin(this->compA, 'A', true);
                }
                if (this->compB) {
                    this->updateCompPin(this->compB, 'B', true);
                }
            }

            if (this->ocrUpdateMode == OCRUpdateMode::Bottom && overflow) {
                // OCRUpdateMode.Top only occurs in Phase Correct modes, handled by phasePwmCount()
                this->ocrA = this->nextOcrA;
                this->ocrB = this->nextOcrB;
                this->ocrC = this->nextOcrC;
            }
            // OCRUpdateMode.Bottom only occurs in Phase Correct modes, handled by phasePwmCount().
            // Thus we only handle TOVUpdateMode.Top or TOVUpdateMode.Max here.
            if (overflow && (this->tovUpdateMode == TOVUpdateMode::Top || TOP == this->MAX)) {
                this->mainCPU->setInterruptFlag(this->defaultOVFInterrupt);
            }
        }
    }

    // Handle manual TCNT updates
    if (this->tcntUpdated) {
        this->tcnt = this->tcntNext;
        this->tcntUpdated = false;
        if ((this->tcnt == 0 && this->ocrUpdateMode == OCRUpdateMode::Bottom) ||
            (this->tcnt == this->getTOP() && this->ocrUpdateMode == OCRUpdateMode::Top)) { // Using getTOP here because the const is limited to the first if in TS...yep, this is true
            this->ocrA = this->nextOcrA;
            this->ocrB = this->nextOcrB;
            this->ocrC = this->nextOcrC;
        }
    }
    // Update the divider if needed
    if (this->updateDivider) {
        const u8 CS = this->getCS();
        const u8 externalClockPin = this->mainConfig->externalClockPin;
        const int newDivider = this->mainConfig->dividers[CS];
        this->lastCycle = newDivider ? this->mainCPU->cycles : 0;
        this->updateDivider = false;
        this->divider = newDivider;

        if (this->mainConfig->externalClockPort && !this->externalClockPort) {
            this->externalClockPort = this->mainCPU->GPIOByPort[this->mainConfig->externalClockPort];
        }
        if (this->externalClockPort) {
            this->externalClockPort->externalClockListeners[externalClockPin] = nullptr;
        }
        if (newDivider) {
            this->mainCPU->addClockEvent(this->mainClockEvent, this->lastCycle + newDivider - this->mainCPU->cycles);
        } else if (this->externalClockPort &&
                   (CS == (u8)ExternalClockMode::FallingEdge || CS == (u8)ExternalClockMode::RisingEdge)) {
            // Assigning the callback to the pin
            this->externalClockPort->externalClockListeners[externalClockPin] = std::make_shared<std::function<void(bool)>>([this](bool value) { this->externalClockCallback(value); });
            this->externalClockRisingEdge = (CS == (u8)ExternalClockMode::RisingEdge);
        }
        return;
    }
    // Reschedule the clock event
    if (reschedule && this->divider) {
        this->mainCPU->addClockEvent(this->mainClockEvent, this->lastCycle + this->divider - this->mainCPU->cycles);
    }
}

void AVRTimer::externalClockCallback(bool value) {
    if (value == this->externalClockRisingEdge) {
        this->count(false, true);
    }
}

void AVRTimer::updateWGMConfig() {

    // Extracting helpful information about the current mode and TCCRA
    const AVRTimerConfig *config = this->mainConfig;
    const auto &wgmModes = (config->bits == 16) ? wgmModes16Bit : wgmModes8Bit;
    const u8 TCCRA = this->mainCPU->data[config->TCCRA];
    const WGMConfig &wgmConfig = wgmModes[this->getWGM()];

    this->timerMode = wgmConfig.mode;
    this->topValue = wgmConfig.topValue;
    this->ocrUpdateMode = wgmConfig.ocrUpdate;
    this->tovUpdateMode = wgmConfig.tovUpdate;

    // Getting the PWM Mode
    bool pwmMode = (this->timerMode == TimerMode::FastPWM ||
                this->timerMode == TimerMode::PWMPhaseCorrect ||
                this->timerMode == TimerMode::PWMPhaseFrequencyCorrect);

    // Update compA
    u8 prevCompA = this->compA;
    this->compA = CompBitsValue((TCCRA >> 6) & 0x3);
    if (this->compA == 1 && pwmMode && !(wgmConfig.flags & OCToggle)) {
        this->compA = CompBitsValue(0);
    }
    if ((prevCompA != 0) != (this->compA != 0)) {
        this->updateCompA(this->compA ? PinOverrideMode::Enable : PinOverrideMode::None);
    }

    // Update compB
    u8 prevCompB = this->compB;
    this->compB = CompBitsValue((TCCRA >> 4) & 0x3);
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
            value++;
            // If we've reached the top value and tcnt isn't updated, update OCR mode and move it along
            if (value == TOP && !tcntUpdated) {
                this->countingUp = false;
                if (this->ocrUpdateMode == OCRUpdateMode::Top) {
                    this->ocrA = this->nextOcrA;
                    this->ocrB = this->nextOcrB;
                    this->ocrC = this->nextOcrC;
                }
            }
        } else { // Otherwise, decrement value
            value--;
            // If we've reached the bottom value and tcnt isn't updated, update OCR mode and move it along
            if (value == 0 && !tcntUpdated) {
                this->countingUp = true;
                this->mainCPU->setInterruptFlag(this->defaultOVFInterrupt);
                if (this->ocrUpdateMode == OCRUpdateMode::Bottom) {
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

void AVRTimer::timerUpdated(u16 value, u16 prevValue) {
    // Extracting relevant constants
    const u16 ocrA = this->ocrA;
    const u16 ocrB = this->ocrB;
    const u16 ocrC = this->ocrC;
    const bool hasOCRC = this->hasOCRC;

    // Detecting overflow
    bool overflow = (prevValue > value);

    // Handle OCR A
    if (((prevValue < ocrA || overflow) && value >= ocrA) || (prevValue < ocrA && overflow)) {
        this->mainCPU->setInterruptFlag(this->defaultOCFAInterrupt);
        if (this->compA) {
            this->updateCompPin(this->compA, 'A');
        }
    }

    // Handle OCR B
    if (((prevValue < ocrB || overflow) && value >= ocrB) || (prevValue < ocrB && overflow)) {
        this->mainCPU->setInterruptFlag(this->defaultOCFBInterrupt);
        if (this->compB) {
            this->updateCompPin(this->compB, 'B');
        }
    }

    // Handle OCR C if available
    if (hasOCRC && (((prevValue < ocrC || overflow) && value >= ocrC) || (prevValue < ocrC && overflow))) {
        this->mainCPU->setInterruptFlag(this->defaultOCFCInterrupt);
        if (this->compC) {
            this->updateCompPin(this->compC, 'C');
        }
    }
}

void AVRTimer::checkForceCompare(u8 value) {
    // Return if the timer is in a PWM mode
    if (this->timerMode == TimerMode::FastPWM ||
        this->timerMode == TimerMode::PWMPhaseCorrect ||
        this->timerMode == TimerMode::PWMPhaseFrequencyCorrect) {
        // FOCnA/FOCnB/FOCnC bits are only active when the WGMn3:0 bits specify a non-PWM mode
        return;
    }

    // Check and update compare pins for A, B, C
    if (value & FOCA) {
        this->updateCompPin(this->compA, 'A');
    }
    if (value & FOCB) {
        this->updateCompPin(this->compB, 'B');
    }
    if (this->mainConfig->compPortC && (value & FOCC)) {
        this->updateCompPin(this->compC, 'C');
    }
}

void AVRTimer::updateCompPin(CompBitsValue compValue, char pinName, bool bottom) {
    // Setting up new value and determining if it's inverting mode and if it's set
    PinOverrideMode newValue = PinOverrideMode::None;
    bool invertingMode = (int(compValue) == 3);
    bool isSet = (this->countingUp == invertingMode);
    // Update the newValue based on the timer mode and compValue
    switch (this->timerMode) {
        case TimerMode::Normal:
        case TimerMode::CTC:
            newValue = compToOverride(compValue);
            break;

        case TimerMode::FastPWM:
            if (int(compValue) == 1) {
                newValue = bottom ? PinOverrideMode::None : PinOverrideMode::Toggle;
            } else {
                newValue = (invertingMode != bottom) ? PinOverrideMode::Set : PinOverrideMode::Clear;
            }
            break;

        case TimerMode::PWMPhaseCorrect:
        case TimerMode::PWMPhaseFrequencyCorrect:
            if (int(compValue) == 1) {
                newValue = PinOverrideMode::Toggle;
            } else {
                newValue = isSet ? PinOverrideMode::Set : PinOverrideMode::Clear;
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

void AVRTimer::updateCompA(PinOverrideMode value){

    // Retrieve the port and pin configuration
    u16 compPortA = this->mainConfig->compPortA;
    u8 compPinA = this->mainConfig->compPinA;

    // Get the corresponding GPIO port from the CPU
    AVRIOPort *port = this->mainCPU->GPIOByPort[compPortA];

    // If the port exists, call the timerOverridePin function
    if (port) {
        port->timerOverridePin(compPinA, value);
    }
}

void AVRTimer::updateCompB(PinOverrideMode value) {
    // Retrieve the port and pin configuration
    u16 compPortB = this->mainConfig->compPortB;
    u8 compPinB = this->mainConfig->compPinB;

    // Get the corresponding GPIO port from the CPU
    AVRIOPort *port = this->mainCPU->GPIOByPort[compPortB];

    // If the port exists, call the timerOverridePin function
    if (port) {
        port->timerOverridePin(compPinB, value);
    }
}

void AVRTimer::updateCompC(PinOverrideMode value) {
    // Retrieve the port and pin configuration
    u16 compPortC = this->mainConfig->compPortC;
    u8 compPinC = this->mainConfig->compPinC;

    // Get the corresponding GPIO port from the CPU
    AVRIOPort *port = this->mainCPU->GPIOByPort[compPortC];

    // If the port exists, call the timerOverridePin function
    if (port) {
        port->timerOverridePin(compPinC, value);
    }
}
