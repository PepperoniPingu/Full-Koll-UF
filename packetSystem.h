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

#define DECODED_FLAG 1
#define RAW_FLAG 0

struct Recording {
  bool decodedFlag;
  IRData recordedIRData;
  unsigned char rawCode[RAW_BUFFER_LENGTH];
  unsigned char rawCodeLength;
};

unsigned int buttonInfoAddress(unsigned char buttonDecimal);

unsigned char readButtonRecordings(Recording recordings[], unsigned char buttonDecimal);

unsigned char readRecordingsOnButton(unsigned char buttonDecimal);

unsigned int readButtonPacketLength(unsigned char buttonDecimal);

void writeButtonPacket(Recording recordings[], unsigned char numberOfRecordings, unsigned char buttonDecimal);

unsigned int scanEmptyEEPROMAddresses(unsigned int bytesRequired);

#endif
