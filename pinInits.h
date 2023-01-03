#ifndef pininits_h
#define pininits_h

#include <Arduino.h>
#include "options.h"
#include <Wire.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>

void setupPinInit();

void I2CPinInit();

void serialPinInit();

void buttonsPinInit();

void receivePinInit();

void serialPinDeInit();

void sendPinInit();

#endif
