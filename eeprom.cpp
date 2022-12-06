#include "eeprom.h"

void writeEEPROM(unsigned int memoryAddress, unsigned char data) {
  do {
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write(memoryAddress >> 8);   // MSB
    Wire.write(memoryAddress & 0xFF);
    Wire.write(data);
  }  while (Wire.endTransmission(true) != 0);
}

unsigned char readEEPROM(unsigned int memoryAddress) {
  unsigned char rdata = 0xFF;
  do {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(memoryAddress >> 8);   // MSB
  Wire.write(memoryAddress & 0xFF);
  } while (Wire.endTransmission(false));
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1, true);
  while (!Wire.available()) {
    Serial.println("Fast");
  }
  rdata = Wire.read();
  while(Wire.available()) {
    Wire.read();
  }
  return rdata;
}
