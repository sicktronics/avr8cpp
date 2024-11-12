export type u8 = number;
export type u16 = number;
export type i16 = number;
export type u32 = number;

// Export keyword: lets result be imported into another module (separate file) at runtime
// type keyword: a type is basically an interface - but can't be changed after creation
// returns bool or void
// takes in value, oldvalue, addr, mask
export type CPUMemoryHook = (value: u8, oldValue: u8, addr: u16, mask: u8) => boolean | void;

export interface CPUMemoryHooks {
    [key: number]: CPUMemoryHook;
  }

// If pass in 16 bit address, returns 8 bit number
export type CPUMemoryReadHook = (addr: u16) => u8;

// Interface for memory read hooks
export interface CPUMemoryReadHooks {
  [key: number]: CPUMemoryReadHook;
}

class CPU {

// data: sram fixed in constructor = 8192 + register space (size to which to initialize array)
// Uint8Array holds 8-bit pos ints
readonly data: Uint8Array = new Uint8Array(8192+256);

// create readHooks, of type empty array of CPU memory read hooks
readonly readHooks: CPUMemoryReadHooks = [];
// create writeHooks, of type empty array of CPU memory hooks
readonly writeHooks: CPUMemoryHooks = []; //step 1

  /**
   * function for reading data
   * pass in addr
   * If addr is geq 32 AND readHooks at addr location exists, then return the data at that location
   */
  readData(addr: number) {
    if (addr >= 32 && this.readHooks[addr]) {
    console.log("the hook did the read successfully");
      return this.readHooks[addr](addr);
    }
    console.log("manual read");
    return this.data[addr];
  }

  /**
   *
   */
  writeData(addr: number, value: number, mask = 0xff) {
    // We get the CPUMemoryHook at the address
    const hook = this.writeHooks[addr];
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
  }
}

// TESTING THE HOOKS

// If no writeHooks is defined, should print the "manual write" message
console.log("commence...");
const cpu = new CPU;
const inverseFlag = true;
const bitSet = 0b00100000;

// only evaluates to true if bitSet is false (bitSet = 00000000)
if(!bitSet){
    console.log("!bitSet is true");
    console.log(~bitSet);
}
if(bitSet){
    console.log("bitSet is true")
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


