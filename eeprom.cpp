#include "eeprom.h"

void writeEEPROM(unsigned int memoryAddress, unsigned char data) {
  bool successfulSend = 0;
  // Try to send data. Can take a couple retries because the EEPROM may be busy with last request. 
  for (int i = 0; i < I2C_RETRIES && !successfulSend; i++) { 
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write(memoryAddress >> 8);   // MSB
    Wire.write(memoryAddress & 0xFF);
    Wire.write(data);
    if (Wire.endTransmission(true) != 0) {
      successfulSend = 1;
    }
  }
}

unsigned char readEEPROM(unsigned int memoryAddress) {
  unsigned char rdata = 0x0;
  do {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(memoryAddress >> 8);   // MSB
  Wire.write(memoryAddress & 0xFF);
  } while (Wire.endTransmission(false)); // Send out bytes but don't send a stop bit
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1, true);
  // Wait for EEPROM to reply. Only try 50 times
  for (int i = 0; i < I2C_RETRIES && !Wire.available(); i++) {
    Serial.println("Stuck");
  }
  if (Wire.available()) {
    rdata = Wire.read(); // Read reply. 
  }

  // Flush the buffer, should not really be needed
  while(Wire.available()) { 
    Wire.read();
  }
  return rdata;
}
