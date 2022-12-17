#ifndef pininits_h
#define pininits_h

#include <Arduino.h>

#define IR_RECEIVE_PIN PIN_PA7
#define IR_SEND_PIN PIN_PA4

#define SHORT_COLUMNS PIN_PB2

#define NO_LED_FEEDBACK_CODE
#define RAW_BUFFER_LENGTH 120 // Number of bytes in saved in a raw recording. Should be at least 112. 
#define EXCLUDE_EXOTIC_PROTOCOLS

#define DEBUG_PRINTING // Not enough memory for both serial and IRSender. Therefore only one can be used at a time. If this is defined, all will work except it won't send any codes. 
#define SERIAL_SPEED 19200 // Need to be low in order to not activate receiver

#include <Wire.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>

void setupPinInit();

void I2CPinInit();

void serialPinInit();

void buttonsPinInit();

void receivePinInit();

void sendPinInit();

#endif
