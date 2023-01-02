#ifndef options_h
#define options_h

#include <Arduino.h>

#define NO_LED_FEEDBACK_CODE
#define RAW_BUFFER_LENGTH 120 // Number of bytes in saved in a raw recording. Should be at least 112. 
#define EXCLUDE_EXOTIC_PROTOCOLS

#define MARK_EXCESS_MICROS 100

#define IR_RECEIVE_PIN PIN_PA7
#define IR_SEND_PIN PIN_PA4

//#define SEND_FAST // Enable this if only sending at 38 kHz and using low clock speed. 

#define SHORT_COLUMNS PIN_PB2

const unsigned char row[5] = {PIN_PB0, PIN_PB1, PIN_PA1, PIN_PA5, PIN_PA3};
const unsigned char column[4] = {PIN_PB3, PIN_PA2, PIN_PA4, PIN_PA6};
#define NUMBER_OF_BUTTONS 20

#define DELAY_BEFORE_SEND 20
#define NUMBER_OF_REPEATS 2U // Results in 3 sent recordings
#define DELAY_BETWEEN_REPEAT 500
#define WAIT_BETWEEN_RECORDINGS 500 // If there are multiple recordings on a button, wait this amount of milliseconds betweeen sending out each recording. 
#define WAIT_AFTER_BUTTON 500

#define DEBUG_PRINTING // Not enough memory for both serial and IRSender. Therefore only one can be used at a time. If this is defined, all will work except it won't send any codes. 
#ifdef DEBUG_PRINTING
  #define DEBUG_EEPROM_PRINTING
#endif
#define SERIAL_SPEED 19200 // Need to be low in order to not activate receiver

#endif
