#include "eeprom.h"

void writeEEPROM(unsigned int memoryAddress, unsigned char data) {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((unsigned char)(memoryAddress >> 8));   // MSB
  Wire.write((unsigned char)(memoryAddress & 0xFF));
  Wire.write(data);
  Wire.endTransmission();
}

unsigned char readEEPROM(unsigned int memoryAddress) {
  unsigned char rdata = 0xFF;
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((unsigned char)(memoryAddress >> 8));   // MSB
  Wire.write((unsigned char)(memoryAddress & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
  while (!Wire.available()) {}
  rdata = Wire.read();
  return rdata;
}
