#include "buttons.h"

// Function to get the bit mask for a button
unsigned long buttonBitMask(unsigned char row, unsigned char column) {
  return 1UL << buttonShiftLeft[row][column];
}

// Remake button 0 to 19 into row and column
void buttonDecimalToMatrice(unsigned char* row, unsigned char* column, unsigned char tempButtonInDecimal) {
  *row = tempButtonInDecimal / 5;
  *column = tempButtonInDecimal % 5;
}
