# Adventures in Simulating an Arduino Uno (ATmega328p)

## Overview

This project is a translation of Uri Shaked's [avr8js repository](https://github.com/wokwi/avr8js) into C++.

The avr8js repository is part of Wokwi. Truly, we are standing on the shoulders of giants here. [Check out the awesome work they're doing for yourself](https://wokwi.com/?utm_medium=blog&utm_source=wokwi-blog).

## Quick Tour

This repository will undergo rapid evolution over the next few months, but here's a quick snapshot of the current state:

- The main branch has been tested on a 2019 Macbook Pro running macOS Sequoia. I am currently adding a few modifications to make the repository cross-compatible with Windows 10 and 11.

- This repository has modules that emulate major components of an Arduino Uno board: the CPU, interrupt handling, general purpose input/output, timers/counters, and opcode parsing.

- There is also some basic "glue code" to test the simulation!
    1. The "compile" module holds intel hex machine code that I generated using the [Arduino CLI](https://github.com/arduino/arduino-cli) which you can install for yourself! Helpful tutorial for getting started [here](https://youtu.be/J-qGn1eEidA?si=B5u5MLCw-tCeKcFA). 

    2. The "index" module is home to main() and is where you can set up listeners to moniter different i/o ports! (Important, say, if you wanted to see when a pin is getting toggled on/off).

    3. The "execute" module runs the main loop of the simulation: repeatedly calling avrInstruction() (which processes the next opcode) and tick() (where the CPU handles clock events and interrupts).

## What's Next

- This library will ultimately be used for the development of a demo for a game called The Robot Underground (TM), which is being built in the Unreal Engine. Checkout it out [here](https://github.com/sicktronics/The-Robot-Underground-Demo)!

-PCM
