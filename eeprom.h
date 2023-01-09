#ifndef eeprom_h
#define eeprom_h

#include <Wire.h>
#include "buttons.h"
#include "options.h"
#include "pinInits.h"

#define EEPROM_I2C_ADDRESS 0x50
#define MAX_EEPROM_ADDRESS 0x1FFFUL
#define I2C_RETRIES 50

void writeEEPROM(unsigned int memoryAddress, unsigned char data);

unsigned char readEEPROM(unsigned int memoryAddress);

#endif
