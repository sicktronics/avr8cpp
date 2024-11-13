// #include <iostream>
// #include "CPU.h"

// using namespace std;

// Test write hook
// bool wHFTest(u8 value, u8 oldValue, u16 address, u8 mask){
//     if(1==1){
//         cpu->data[address] = value;
//         return true;
//     }
//     else {
//         return false;
//     }
// }

// Test read hook
// u8 rHFTest(u16 address){
//     return cpu->data[address];
// }

/*
For testing clock stuff
*/

// // set up a test cpu const cpu = new CPU(new Uint16Array(1024), 8192);
// std::vector<u16> testPM(1024);

// CPU *cpu = new CPU(testPM);

// // Set up a type of array, ITestEvent, with two numbers: expected cycles vs actual cycles
// // set up an array of ITestEvents (const events: ITestEvent[] = [];)
// std::vector<int> events;

// // Define three different callback functions
// void callback1(){
//     std::cout << "callback 1 exectuing!!" << std::endl;
//     events.push_back(1);
//     events.push_back(cpu->cycles);
//     std::cout << "Num cycles taken: " << int(cpu->cycles) << std::endl;
// }
// void callback4(){
//     std::cout << "callback 4 exectuing!!" << std::endl;
//     events.push_back(4);
//     events.push_back(cpu->cycles);
//     std::cout << "Num cycles taken: " << int(cpu->cycles) << std::endl;
// }
// void callback10(){
//     std::cout << "callback 10 exectuing!!" << std::endl;
//     events.push_back(10);
//     events.push_back(cpu->cycles);
//     std::cout << "Num cycles taken: " << int(cpu->cycles) << std::endl;
// }
// void defaultCallback1(){}
// void defaultCallback2(){}
// void defaultCallback3(){}

// int main(int argc, char* argv[]) {

//     cout << "tests commence!"<< endl;

    // ✅ Test1: SP reset from CPU reset function
    // std::cout<< "expected value: " << cpu->SRAM_BYTES + REGISTER_SPACE - 1 << std::endl;
    // std::cout<< int(cpu->getSP()) << endl;

    /* * * * * * * */

    // ✅ TEST 6: confirm that CPU returns false if the provided clock event is not scheduled
    // AVRClockEventCallback callbackArr[11];
    // AVRClockEventCallback defaultCB1 = defaultCallback1;
    // AVRClockEventCallback defaultCB2 = defaultCallback2;
    // AVRClockEventCallback defaultCB3 = defaultCallback3;
    // // add a couple of clock events, each taking 4 and 10 cycles, respectively. Store them in consts
    // // const event4 = cpu.addClockEvent(() => 0, 4);
    // callbackArr[4] = cpu->addClockEvent(defaultCallback1, 4);
    // // const event10 = cpu.addClockEvent(() => 0, 10);
    // callbackArr[10] = cpu->addClockEvent(defaultCallback1, 10);
    // // add one more clock event, for good measure
    // // cpu.addClockEvent(() => 0, 1);
    // callbackArr[1] = cpu->addClockEvent(defaultCallback1, 1);

    // // clear event4 and event10 twice. First time should = true for successful removal, 
    // cout << cpu->clearClockEvent(callbackArr[4]) << endl;
    // cout << cpu->clearClockEvent(callbackArr[10]) << endl;
    // // second time should = false because they were already removed... 
    // cout << cpu->clearClockEvent(callbackArr[4]) << endl;
    // cout << cpu->clearClockEvent(callbackArr[10]) << endl;

    /* * * * * * * */

    // ✅ TEST 5: confirm that CPU removes the given clock event

    // set up an empty array of AVRClockEventCallback called callbacks
    // set up a collection of callbacks
    // AVRClockEventCallback callbackArr[11];
    // AVRClockEventCallback c1 = callback1;
    // AVRClockEventCallback c4 = callback4;
    // AVRClockEventCallback c10 = callback10;

    // // addClockEvent where callback is defined as pushing i, cpu cycle pairs onto the events array, 
    // // pass in i as cycles param
    // // store result in callbacks[i]
    //     // for (const i of [1, 4, 10]) {
    //     //   callbacks[i] = cpu.addClockEvent(() => events.push([i, cpu.cycles]), i);
    //     // }
    // callbackArr[1] = cpu->addClockEvent(c1, 1);
    // callbackArr[4] = cpu->addClockEvent(c4, 4);
    // callbackArr[10] = cpu->addClockEvent(c10, 10);

    // // call clearClockEvent and pass it callback[4]
    // cpu->clearClockEvent(callbackArr[4]);

    //  // loop 10 times:
    //     // increment cpu cycles
    //     // call tick
    // for (int i = 0; i < 10; i++) {
    //     cpu->cycles++;
    //     cpu->tick();
    //     cout << "tick'd"<< endl;
    // }
    // //  check the events array, should = [1, 1], [10, 10]
    // for(int i = 0; i < events.size(); i++) {
    //         std::cout << events[i] << " ";
    // }
    //  std::cout << std::endl;

    /* * * * * * * */

    // ✅ TEST 4: confirm that CPU updates the number of cycles for the given clock event

    // // set up a collection of callbacks
    // AVRClockEventCallback callbackArr[11];
    // AVRClockEventCallback c1 = callback1;
    // AVRClockEventCallback c4 = callback4;
    // AVRClockEventCallback c10 = callback10;

    // callbackArr[1] = cpu->addClockEvent(c1, 1);
    // callbackArr[4] = cpu->addClockEvent(c4, 4);
    // callbackArr[10] = cpu->addClockEvent(c10, 10);

    // // update the callbacks at a few locations with updated cycle counts
    // // cpu.updateClockEvent(callbacks[4], 2); - new cycle number = 2
    // // cpu.updateClockEvent(callbacks[1], 12);
    // cpu->updateClockEvent(callbackArr[4], 2);
    // cpu->updateClockEvent(callbackArr[1], 12);

    //  // loop 14 times:
    //     // increment cpu cycles
    //     // call tick
    // for (int i = 0; i < 14; i++) {
    //     cpu->cycles++;
    //     cpu->tick();
    //     // cout << "tick'd"<< endl;
    // }

    // // check the events array, should = [4, 2], [10, 10], [1, 12],
    // for(int i = 0; i < events.size(); i++) {
    //         std::cout << events[i] << " ";
    // }
    //  std::cout << std::endl;
    
    /* * * * * * * */

    /* ✅ TEST2: confirm that queued events execute after the given number of cycles has passed */
    /* Setting up our callback functions as AVRClockEventCallbacks*/
    // AVRClockEventCallback c1 = callback1;
    // AVRClockEventCallback c4 = callback4;
    // AVRClockEventCallback c10 = callback10;

    // cout << "created the three callbacks"<< endl;

    // // addClockEvent where callback is defined as pushing i, cpu cycle pairs onto the events array, 
    // // pass in i as cycles param
    // // for (const i of [1, 4, 10]) { 
    // //    cpu.addClockEvent(() => events.push([i, cpu.cycles]), i);
    // //  }

    // /*
    // * let's try matching more precisely by looping!!
    // */

    // // Put in reverse order for TEST 3 ✅
    // cpu->addClockEvent(c10, 10);
    // cpu->addClockEvent(c4, 4);
    // cpu->addClockEvent(c1, 1);

    // // loop 10 times:
    // //     increment cpu cycles
    // //     call tick

    // cout << "added three clock events!!"<< endl;

    // for (int i = 0; i < 10; i++) {
    //     cpu->cycles++;
    //     cpu->tick();
    //     cout << "tick'd"<< endl;
    // }
    // cout << "reached the for loop"<< endl;
    // // event array should look like [1, 1],[4, 4],[10, 10]
    // for(int i = 0; i < events.size(); i++) {
    //         std::cout << events[i] << " ";
    // }
    //  std::cout << std::endl;


    /* * * * * * * */

    // std::vector<u16> testPM;
    // testPM.push_back(1);
    // testPM.push_back(2);
    // testPM.push_back(3);
    // testPM.push_back(4);

    // CPU *cpu = new CPU(testPM);

    // cpu->pendingInterrupts[0] = new AVRInterruptConfig;
    // for (int i = 0; i < MAX_INTERRUPTS; i++){
    //     std::cout << cpu->pendingInterrupts[i] << std::endl;
    // }
    // std::cout << "reset" << std::endl;
    // cpu->reset();

    // ✅ Test SP reset from CPU reset function
    // std::cout<< "expected value: " << cpu->SRAM_BYTES + REGISTER_SPACE - 1 << std::endl;
    // std::cout<< int(cpu->getSP()) << endl;

    // ✅ test getUint16LittleEndian() and setUint16LittleEndian - yes!!!
    // cpu->setUint16LittleEndian(10, 0b1000000011111111);
    // std::cout << "the value: " << int(0b1000000011111111) << std::endl;
    // std::cout << int(cpu->data[10]) << std::endl;
    // std::cout << int(cpu->data[11]) << std::endl;
    // std::cout << int(cpu->getUint16LittleEndian(10)) << std::endl;


    

    /* Interrupts */

    // Clearing interrupt by flag ✅
    // AVRInterruptConfig *tI = new AVRInterruptConfig;
    // tI->flagRegister = 8;
    // cpu->data[tI->flagRegister] = 0b11111011;
    // tI->flagMask = 0b10000000;
    // cpu->clearInterruptByFlag(tI, 0b11111111);

    // Clearing interrupt ✅
    // AVRInterruptConfig *tI = new AVRInterruptConfig;
    // AVRInterruptConfig *tI2 = new AVRInterruptConfig;
    // tI2->address = 50;
    // AVRInterruptConfig *tI3 = new AVRInterruptConfig;
    // tI3->address = 51;
    // tI->flagRegister = 8;
    // cpu->data[tI->flagRegister] = 0b11111111;
    // tI->flagMask = 0b01111111;
    // tI->address = 2;
    // cpu->queueInterrupt(tI);
    // // cpu->queueInterrupt(tI2);
    // // cpu->queueInterrupt(tI3);
    // cpu->clearInterrupt(tI, true);

    // Updating interrupts enabled ✅
    // AVRInterruptConfig *tI = new AVRInterruptConfig;
    // tI->enableMask = 0b10000000;
    // tI->flagRegister = 8;
    // cpu->data[tI->flagRegister] = 0b00001000;
    // tI->flagMask = 0b11111111;
    // tI->inverseFlag = false;
    // Should enter first IF statement
    // bitSet is zero, inverse false, should not queue anything
    // cpu->updateInterruptsEnabled(tI, 0b11111111);


    // Setting interrupt flag ✅
    // AVRInterruptConfig *tI = new AVRInterruptConfig;
    // tI->inverseFlag = false;
    // tI->address = 10;

    // tI->enableRegister = 7;
    // cpu->data[tI->enableRegister] = 0b10101010;
    // tI->enableMask = 0b00000001;

    // tI->flagRegister = 8;
    // cpu->data[tI->flagRegister] = 0b00001000;
    // tI->flagMask = 0b11110111;

    // inverse flag true
    // data[flagReg] should be 00001000 & ~11110111 = 00001000 - YEP
    // enReg contents != enMsk --> interrupt should not be queued - YEP

    // cpu->setInterruptFlag(tI);
    // cout << "checking the contents of the flagRegister: " << int(cpu->data[tI->flagRegister]) << endl;
    // cout << tI << endl;
    // cout << "checking if the interrupt got queued: " << cpu->pendingInterrupts[tI->address] << endl;

    // inverse flag false
    // data[flagReg] should be 11110111 | 00001000 = 11111111 - YEP
    // enReg contents == enMsk --> interrupt should be queued - YEP

    // Queueing an interrupt ✅
    // AVRInterruptConfig *testInterrupt1 = new AVRInterruptConfig;
    // AVRInterruptConfig *testInterrupt2 = new AVRInterruptConfig;
    // AVRInterruptConfig *testInterrupt3 = new AVRInterruptConfig;
    // cout << "first ! "<< endl;
    // testInterrupt1->address = 120;
    // cpu->queueInterrupt(testInterrupt1);
    // cout << "second ! "<< endl;
    // testInterrupt2->address = 90;
    // cpu->queueInterrupt(testInterrupt2); // should only trigger the first message
    // cout << "third ! "<< endl;
    // testInterrupt3->address = 127;
    // cpu->queueInterrupt(testInterrupt3); // should only trigger the third message
    
    /* SREG ✅ */

    // cpu->writeData(95, 0x82);
    // cout << "SREG is currently: " << int(cpu->getSREG()) << endl;
    // cout << "Interrupts are enabled?: " << cpu->getInterruptsEnabled() << endl;

    /* Data writing / reading ✅ */

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

// }