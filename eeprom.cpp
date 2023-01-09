#include "eeprom.h"

void writeEEPROM(unsigned int memoryAddress, unsigned char data) {  
  bool successfulSend = 0;

  #ifdef DEBUG_PRINTING
    Wire.end();
    serialPinInit();
    Serial.print("W A");
    Serial.print(memoryAddress, DEC);
    Serial.print(" D");
    Serial.println(data, DEC);
    serialPinDeInit();
    I2CPinInit();
  #endif

  do {
    // Try to send data. Can take a couple retries because the EEPROM may be busy with last request. 
    for (int i = 0; i < I2C_RETRIES && !successfulSend; i++) { 
      Wire.beginTransmission(EEPROM_I2C_ADDRESS);
      Wire.write((unsigned char)(memoryAddress >> 8));   // MSB
      Wire.write((unsigned char) memoryAddress);
      Wire.write(data);
      if (Wire.endTransmission(true) == 0) {
        successfulSend = 1;
      }
    }
  } while(readEEPROM(memoryAddress) != data); // If there is no direct read efterwards the EEPROM will stay in writing mode and the data can accidentally become overridden
}

unsigned char readEEPROM(unsigned int memoryAddress) {
  unsigned char rdata = 0x0;
  do {
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((unsigned char)(memoryAddress >> 8));   // MSB
    Wire.write((unsigned char) memoryAddress);
  } while (Wire.endTransmission(false) != 0); // Send out bytes but don't send a stop bit
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1, true);
  // Wait for EEPROM to reply. Only try 50 times
  for (int i = 0; i < I2C_RETRIES && !Wire.available(); i++) {}
  if (Wire.available()) {
    rdata = Wire.read(); // Read reply. 
  }

  // Flush the buffer, should not really be needed
  /*while(Wire.available()) { 
    Wire.read();
  }*/
  return rdata;
}
