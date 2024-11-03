#include <iostream>
#include "CPU.h"

using namespace std;

CPU *cpu = new CPU;

bool wHFTest(u8 value, u8 oldValue, u16 address, u8 mask){
    if(1==1){
        cpu->data[address] = value;
        return true;
    }
    else {
        return false;
    }
}

u8 rHFTest(u16 address){
    return cpu->data[address];
}

int main(int argc, char* argv[]) {

    cout << "tests commence!"<< endl;

    // TEST 1: Manual write to data[]
    // want to store number "19" at data location 0
    // cpu->writeData(32, 19);
    // cout << "test: " << int(cpu->data[0]) << endl;

    // TEST 2: Manual read to data[]
    // int result = cpu->readData(0);
    // cout << "data read: " << result << endl;

    // TEST 3: Define sample writeHook and use it to write to data
    // Define a writeHookFunction (above) and have it live at loc 0 in the array
    // cpu->writeHookFunctions[32] = *wHFTest;
    // // Ok, now the hook should do the data writing, not the hard-write.
    // cpu->writeData(32, 17);
    // cout << "data at location 32: " << int(cpu->data[32]) << endl;

    // TEST 4: Define sample readHook and use it to write to data
    // cpu->readHookFunctions[32] = *rHFTest;
    // int result = cpu->readData(32);
    // cout << "data at location 32: " << result << endl;

}