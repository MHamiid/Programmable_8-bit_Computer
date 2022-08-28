#include "EEPROM_Programmer.h"

void setup () {

  initPins();
   
   Serial.begin(57600);
   
   
   // 4-bit hex decoder for common cathode 7-segment display
   byte sevenSegmentsDigitsCode[] = { 0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b};

   Serial.println("[!] Programming ones place");
   for(int value = 0; value <= 255; value++)
   {
      writeEEPROMByte(value, sevenSegmentsDigitsCode[value % 10]);                                      // (ones-place)
   }
   
   Serial.println("[!] Programming tens place");
   for(int value = 0; value <= 255; value++)
   {
      writeEEPROMByte(value + 256, sevenSegmentsDigitsCode[(value / 10) % 10 ]);               // (tens-place)
   }
   
  Serial.println("[!] Programming hundreds place");
  for(int value = 0; value <= 255; value++)
   {
      writeEEPROMByte(value + 512, sevenSegmentsDigitsCode[value / 100 ]);                        // (hundreds-place)
   }
   
  Serial.println("[!] Programming sign");
  for(int value = 0; value <= 255; value++)
   {
      writeEEPROMByte(value + 768,0);               									    // Sign
   }
  
  /* For twos complement representation */
  
  Serial.println("[!] Programming ones place for twos complement");
  for(int value = -128; value <= 127; value++)
   {
      writeEEPROMByte((byte)value + 1024, sevenSegmentsDigitsCode[abs(value) % 10]);                   // (ones-place)
   }
  
   Serial.println("[!] Programming tens place for twos complement");
   for(int value = -128; value <= 127; value++)
   {
      writeEEPROMByte((byte)value +1280, sevenSegmentsDigitsCode[abs(value / 10) % 10]);           // (tens-place)
   }
  
   Serial.println("[!] Programming hundreds place for twos complement");
   for(int value = -128; value <= 127; value++)
   {
      writeEEPROMByte((byte)value + 1536, sevenSegmentsDigitsCode[abs(value / 100)]);                // (hundreds-place)
   }
  
   Serial.println("[!] Programming sign for twos complement");
   for(int value = -128; value <= 127; value++)
   {
      byte writeData = 0;    // Positive
      
      if(value < 0)   // Negative
	 writeData = 0x01;    // Outputs (-) on the 7-segments
	 
      writeEEPROMByte((byte)value + 1792, writeData);                                   			            // Sign
   }
  
   Serial.println("[!] Finished EEPROM programming");
   
   
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
