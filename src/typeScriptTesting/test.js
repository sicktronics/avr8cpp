"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var CPU = /** @class */ (function () {
    function CPU() {
        // data: sram fixed in constructor = 8192 + register space (size to which to initialize array)
        // Uint8Array holds 8-bit pos ints
        this.data = new Uint8Array(8192 + 256);
        // create readHooks, of type empty array of CPU memory read hooks
        this.readHooks = [];
        // create writeHooks, of type empty array of CPU memory hooks
        this.writeHooks = []; //step 1
    }
    /**
     * function for reading data
     * pass in addr
     * If addr is geq 32 AND readHooks at addr location exists, then return the data at that location
     */
    CPU.prototype.readData = function (addr) {
        if (addr >= 32 && this.readHooks[addr]) {
            console.log("the hook did the read successfully");
            return this.readHooks[addr](addr);
        }
        console.log("manual read");
        return this.data[addr];
    };
    /**
     *
     */
    CPU.prototype.writeData = function (addr, value, mask) {
        if (mask === void 0) { mask = 0xff; }
        // We get the CPUMemoryHook at the address
        var hook = this.writeHooks[addr];
        if (hook) {
            // If a hook for that address exists and successfully handles the write operation, it prevents the actual write from occurring. 
            if (hook(value, this.data[addr], addr, mask)) { // Calling the hook returns TRUE if the writeHooks already too care of the writing
                console.log("the hook did the write successfully");
                return;
            }
        }
        // Otherwise, it writes the value directly. 
        console.log("manual write");
        this.data[addr] = value;
    };
    return CPU;
}());
// TESTING THE HOOKS
// If no writeHooks is defined, should print the "manual write" message
console.log("commence...");
var cpu = new CPU;
var inverseFlag = true;
var bitSet = 32;
// only evaluates to true if bitSet is false (bitSet = 00000000)
if (!bitSet) {
    console.log("!bitSet is true");
    console.log(~bitSet);
}
if (bitSet) {
    console.log("bitSet is true");
}
// if (inverseFlag ? !bitSet : bitSet) {
//     // Do stuff
// }
// Should push use writeHooks to write
// cpu.writeHooks[32] = (value: u8, oldValue, addr, mask) => {
//     cpu.data[addr] = value;
//     return true;
//   };
// cpu.readHooks[32] = (addr) => {
//     return cpu.data[addr];
//   };
// // call write data
// cpu.writeData(32, 89);
// cpu.readData(32);
// console.log("data at address 32: ", cpu.data[32]);
