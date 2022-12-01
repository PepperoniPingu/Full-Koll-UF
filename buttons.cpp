#include "buttons.h"

// Function to get the bit mask for a button
unsigned long buttonBitMask(unsigned char row, unsigned char column) {
  return 1UL << buttonShiftLeft[row][column];
}
