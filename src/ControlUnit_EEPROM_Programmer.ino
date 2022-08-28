#include "EEPROM_Programmer.h"

/*Control signals bit position*/
#define HLT    0b1000000000000000                                  // Halt
#define MI     0b0100000000000000                                  // Memory address register in
#define RI     0b0010000000000000                                  // Memory in (RAM in)
#define RO     0b0001000000000000                                  // Memory out (RAM out)
#define IO     0b0000100000000000                                  // Instruction register out
#define II     0b0000010000000000                                  // Instruction register int
#define AI     0b0000001000000000                                  // A register in
#define AO     0b0000000100000000                                  // A register out
#define EO     0b0000000010000000                                  // ALU register out (Sum out)
#define SU     0b0000000001000000                                  // Subtract
#define BI     0b0000000000100000                                  // B register in
#define OI     0b0000000000010000                                  // Output (Display) register in
#define CE     0b0000000000001000                                  // Program counter enable
#define CO     0b0000000000000100                                  // Program counter register out
#define J      0b0000000000000010                                  // Program counter in (Jump)
#define FI     0b0000000000000001                                  // Flags Register in
/**/
#define FLAGS_C0Z0 0
#define FLAGS_C1Z0 1
#define FLAGS_C0Z1 2
#define FLAGS_C1Z1 3
/*Opcodes*/
#define JC 0b0111                                                  // Jump Carry
#define JZ 0b1000                                                  // Jump Zero


// Defining 16 instructions each has 16-bit control word microcode for the possible 8 steps of the microcode counter (2^3)
const uint16_t instructions_template[16][8] = { 
	/*Step Counter*/
  /* 100 : (last counter step before resetting)*/
	/*000,        001,             010,        011,        100,                    101, 110, 111 */
	/**/
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(0000) ===> NOP      "(No operation) :   Doing just the fetch cycle"
  {  MI | CO,    RO | II | CE,    IO | MI,    RO | AI,     0,                        0, 0, 0  },                       // Opcode(0001) ===> LDA      "(LOAD A) :         Load data from memory at the specified address into the A resgister"
  {  MI | CO,    RO | II | CE,    IO | MI,    RO | BI,     EO | AI | FI,             0, 0, 0  },                       // Opcode(0010) ===> ADD      "(ADD) :            Load data from memory at the specified address into the B register, and load the sum into the A register and load the flags status into the flags register before the output register is changed"
  {  MI | CO,    RO | II | CE,    IO | MI,    RO | BI,     EO | AI | SU | FI,        0, 0, 0  },         			         // Opcode(0011) ===> SUB      "(SUBTRACT) :       Load data from memory at the specified address into the B register, set the subtract bit and load the sum into the A register and load the flags status into the flags register before the output register is changed"
  {  MI | CO,    RO | II | CE,    IO | MI,    AO | RI,     0,                        0, 0, 0  },                       // Opcode(0100) ===> STA      "(STORE A) :        Load data from the A register into the memory at the specified address" 
  {  MI | CO,    RO | II | CE,    IO | AI,    0,           0,                        0, 0, 0  },                       // Opcode(0101) ===> LDI      "(LOAD IMMEDIATE) : Load the operand (4-bits) into the A register"
  {  MI | CO,    RO | II | CE,    IO |  J,    0,           0,                        0, 0, 0  },                       // Opcode(0110) ===> JMP      "(JUMP) :           Load the operand (4-bits address we want to jump to) into the program counter to be excuted the next clock cycle"
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(0111) ===> JC       "(JUMP CARRY) :     If the carry flag is set, Load the operand (4-bits address we want to jump to) into the program counter to be excuted the next clock cycle"
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(1000) ===> JZ       "(JUMP ZERO) :      If the zero flag is set, Load the operand (4-bits address we want to jump to) into the program counter to be excuted the next clock cycle"
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(1001) ===> NOP
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(1010) ===> NOP
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(1011) ===> NOP
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(1100) ===> NOP
  {  MI | CO,    RO | II | CE,    0,          0,           0,                        0, 0, 0  },                       // Opcode(1101) ===> NOP
  {  MI | CO,    RO | II | CE,    AO | OI,    0,           0,                        0, 0, 0  },                       // Opcode(1110) ===> OUT      "load data from A register to the output register"
  {  MI | CO,    RO | II | CE,    HLT,        0,           0,                        0, 0, 0  },                       // Opcode(1111) ===> HLT      "Stop the computer clock"
};

/* [Flags][Instructions][Steps] */
uint16_t instructions[4][16][8];

void initInstructions()
{

  // For when the Carry Flag = 0 and Zero Flag = 0 
  memcpy(instructions[FLAGS_C0Z0], instructions_template, sizeof(instructions_template));

  // For when the Carry Flag = 1 and Zero Flag = 0 
  memcpy(instructions[FLAGS_C1Z0], instructions_template, sizeof(instructions_template));
  /* Overridding values */ 
  // Set the microcode(control word) Where the zero bit is set and the instruction is JC(Jump Carry) at step 2 (after the fetch cycle has been executed)
  instructions[FLAGS_C1Z0][JC][2] = IO | J;   // Load the operand (4-bits address we want to jump to) into the program counter to be excuted the next clock cycle

  // For when the Carry Flag = 0 and Zero Flag = 1 
  memcpy(instructions[FLAGS_C0Z1], instructions_template, sizeof(instructions_template));
  /* Overridding values */ 
  // Set the microcode(control word) Where the zero flag is set and the instruction is JZ(Jump Zero) at step 2 (after the fetch cycle has been executed)
  instructions[FLAGS_C0Z1][JZ][2] = IO | J;   // Load the operand (4-bits address we want to jump to) into the program counter to be excuted the next clock cycle


  // For when the Carry Flag = 1 and Zero Flag = 1 
  memcpy(instructions[FLAGS_C1Z1], instructions_template, sizeof(instructions_template));
  /* Overridding values */ 
  instructions[FLAGS_C1Z1][JC][2] = IO | J;
  instructions[FLAGS_C1Z1][JZ][2] = IO | J;
}

void setup () {

  initInstructions();

  initPins();

   Serial.begin(57600);
   
   

   Serial.println("[!] Programming Instructions For The EEPROMs");
   for(int address = 0; address < 1024; address++)
   {
    /*
    *   Each address is 10 bits (FFBOOOOSSS)
    *                                          F ===> Flags   |   B ===> Byte Selector   |   O ===> Opcode/Instructions   |  S  ===> Microcode step
    *
    *
    */
    int flags           = (address & 0b1100000000) >> 8;                // Get the flags bits, and shift them to the right to be a number in range [0-3]
    int byteSelector    = (address & 0b0010000000) >> 7;                // Get the byte selector bit, and shift them to the right to be a number in range [0-1]
    int opcode          = (address & 0b0001111000) >> 3;                // Get the opcode/instruction bits, and shift them to the right to be a number in range [0-15]
    int step            = (address & 0b0000000111);                     // Get the step bits which is a number in range [0-7]


    if(byteSelector)
    {
      // Write the lower byte (the first 8-bits of the 16-bit microcode (control word))
      writeEEPROMByte(address, instructions[flags][opcode][step]);          // Giving the function the full 16-bit value will result in writing only the lower byte (8-bits)
    }
    else
    {
      // 
      // Write the top byte (the last 8-bits of the 16-bit microcode (control word))
      writeEEPROMByte(address, (instructions[flags][opcode][step] >> 8));   // Shift the 16-bit microcode by 8-bits to the right to get the top byte 
    }
    
   }

   
  
   Serial.println("[!] Finished EEPROM programming");
   
   //printEEPROMContent();
   
    /*
     * set the Arduino data pins to input after programming to avoid outputting data to the pins and result in contention with the EEPROM output data
    */
    // Iterate over  the data pins 
   for(int pin = EEPROM_D7_PIN; pin >= EEPROM_D0_PIN; pin--)
   {
      // Set the pin mode to input
      // NOTE: The order where setting the pins mode to input before EEPROM output is enabled in setEEPROMAddress()
      //, to avoid the EEPROM and arduino writting to the data line at the same time resulting in contention
      pinMode(pin, INPUT);
   }
   
    /*
     *     Enable the EEPROM output bit
    */
   setEEPROMAddressAndOutputEnable(0, true);
   
}

void loop() {
   
  
}
