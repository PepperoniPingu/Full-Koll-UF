#ifndef packetsystem_h
#define packetsystem_h

#include <Arduino.h>
#include "options.h"
#include <Wire.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>
#include "buttons.h"
#include "eeprom.h"
#ifdef DEBUG_PRINTING
  #include "pinInits.h"
#endif

unsigned int buttonInfoAddress(unsigned char buttonDecimal);

unsigned char readButtonPacket(IRData buttonPacketPtr[], unsigned char buttonDecimal);

unsigned char readRecordingsOnButton(unsigned char buttonDecimal);

unsigned char readButtonPacketLength(unsigned char buttonDecimal);

void writeButtonPacket(IRData tempIRData[], unsigned char recordings, unsigned char buttonDecimal);

unsigned int scanEmptyEEPROMAddresses(unsigned int bytesRequired);

#endif
