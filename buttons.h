#ifndef buttons_h
#define buttons_h

#include "options.h"

// Definitions for amount of steps to shift left to get to the bit position
#define btn00ShiftLeft 0
#define btn01ShiftLeft 1
#define btn02ShiftLeft 2
#define btn03ShiftLeft 3
#define btn10ShiftLeft 4
#define btn11ShiftLeft 5
#define btn12ShiftLeft 6
#define btn13ShiftLeft 7
#define btn20ShiftLeft 8
#define btn21ShiftLeft 9
#define btn22ShiftLeft 10
#define btn23ShiftLeft 11
#define btn30ShiftLeft 12
#define btn31ShiftLeft 13
#define btn32ShiftLeft 14
#define btn33ShiftLeft 15
#define btn40ShiftLeft 16
#define btn41ShiftLeft 17
#define btn42ShiftLeft 18
#define btn43ShiftLeft 19

// Function to get the bit mask for a button
unsigned long buttonBitMask(unsigned char row, unsigned char column);

// Function to get the bit mask for a button
unsigned long buttonBitMask(unsigned char buttonDecimal);

// Remake button 0 to 19 into row and column
void buttonDecimalToMatrice(unsigned char* row, unsigned char* column, unsigned char tempButtonInDecimal);

// Remake row and column to 0 to 19
unsigned char buttonMatriceToDecimal(unsigned char row, unsigned char column);

// Function for reading the buttons
void scanMatrix(unsigned long* buttonStates, unsigned long* lastButtonStates, char *lastPressedButton);

#endif
