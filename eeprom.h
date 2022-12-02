#ifndef eeprom_h
#define eeprom_h

#include <Wire.h>
#include "buttons.h"

#define EEPROM_I2C_ADDRESS 0x50

#define btn00TypeAddress 0x00;
#define btn01TypeAddres

void writeEEPROM(unsigned int memoryAddress, unsigned char data);

unsigned char readEEPROM(unsigned int memoryAddress);

unsigned int buttonInfoAddress(unsigned char row, unsigned char column);

unsigned int buttonPacketAddress(unsigned char row, unsigned char column);

#endif
