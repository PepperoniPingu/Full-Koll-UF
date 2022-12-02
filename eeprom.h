#ifndef eeprom_h
#define eeprom_h

#include <Wire.h>

#define EEPROM_I2C_ADDRESS 0x50

void writeEEPROM(unsigned int memoryAddress, unsigned char data);

unsigned char readEEPROM(unsigned int memoryAddress);

#endif
