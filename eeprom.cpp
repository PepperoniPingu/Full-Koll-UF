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

// Each button has a permanent address for info. The info contains another address to where a "button packet" is found. A button packet contains 
// number of signals, protocol type and protocol data are stored. The info is 2 bytes which are the address of the button packet. 
// This function returns the address for the permanent address for the button info. 
unsigned int buttonInfoAddress(unsigned char row, unsigned char column) {
  return 2 * buttonShiftLeft[row][column];
}

// Function to return the dynamic address of a button packet. 
unsigned int buttonPacketAddress(unsigned char row, unsigned char column) {
  unsigned int tempButtonPacketAddress;
  tempButtonPacketAddress = readEEPROM(buttonInfoAddress(row, column)) << 8;
  tempButtonPacketAddress |= readEEPROM(buttonInfoAddress(row, column) + 1);
  return tempButtonPacketAddress;
}
