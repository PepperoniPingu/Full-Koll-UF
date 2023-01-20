#ifndef options_h
#define options_h

#include <Arduino.h>

#define NO_LED_FEEDBACK_CODE
#define RAW_BUFFER_LENGTH 160 // Number of bytes in saved in a raw recording. Should be at least 112. 
#define EXCLUDE_EXOTIC_PROTOCOLS

#define MARK_EXCESS_MICROS 20 // Default 20
#define TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING 25 // Default 25
#define RECORD_GAP_MICROS 15000 // Default 5000

#define IR_RECEIVE_PIN PIN_PA7
#define IR_SEND_PIN PIN_PA4

//#define SEND_FAST // Enable this if only sending at 38 kHz and using low clock speed. 

#define SHORT_COLUMNS PIN_PB2

const unsigned char row[5] = {PIN_PB0, PIN_PB1, PIN_PA1, PIN_PA5, PIN_PA3};
const unsigned char column[4] = {PIN_PB3, PIN_PA2, PIN_PA4, PIN_PA6};
#define NUMBER_OF_BUTTONS 20

#define BUTTON_RESET_MILLIS 2000UL // The amount of time to hold down a button in recording mode to delete all the recordings on it

#define DELAY_BEFORE_SEND 20 // Delay between I2C reading and IR sending. Important since the I2C reading interferes with the IR LED
#define NUMBER_OF_REPEATS 0U // Results in 3 sent recordings
#define DELAY_BETWEEN_REPEAT 700
#define WAIT_BETWEEN_RECORDINGS 100 // If there are multiple recordings on a button, wait this amount of milliseconds betweeen sending out each recording
#define WAIT_AFTER_BUTTON 200

#define FORCE_SLEEP_AFTER 10000 // After this amount of milliseconds, force the microcontroller in to sleep even if buttons are depressed

//#define DEBUG_PRINTING // Not enough memory for both serial and IRSender. Therefore only one can be used at a time. If this is defined, all will work except it won't send any codes. 
#ifdef DEBUG_PRINTING
  //#define DEBUG_EEPROM_PRINTING
  #define SERIAL_SPEED 19200 // Need to be low in order to not activate receiver
#endif

#endif
