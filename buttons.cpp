#include "buttons.h"

// Function to get the bit mask for a button
unsigned long buttonBitMask(unsigned char tempRow, unsigned char tempColumn) {
  return 1UL << buttonMatriceToDecimal(tempRow, tempColumn);
}

unsigned long buttonBitMask(unsigned char buttonDecimal) {
  return 1UL << buttonDecimal;
}

// Remake button 0 to 19 into row and column
void buttonDecimalToMatrice(unsigned char* tempRow, unsigned char* tempColumn, unsigned char tempButtonInDecimal) {
  *tempRow = tempButtonInDecimal / 4;
  *tempColumn = tempButtonInDecimal % 4;
}

// Remake row and column to 0 to 19
unsigned char buttonMatriceToDecimal(unsigned char tempRow, unsigned char tempColumn) {
  return tempRow * sizeof(column) + tempColumn;
}

// Function for reading the buttons
void scanMatrix(unsigned long* buttonStates, unsigned long* lastButtonStates, char *lastPressedButton) {
  // Copy last button state
  *lastButtonStates = *buttonStates;

  unsigned long tempButtonStates;

  // Loop atleast two times and make sure the buttons have the same states both times. Needed because of hardware bug that can make it register wrong button presses if last 
  // column of buttons is pressed in the middle of the scanning process.
  do {
    tempButtonStates = *buttonStates;
  
    // Reset button states
    *buttonStates = 0;
    
    // Scan matrix and update array
    for (unsigned char i = 0; i < sizeof(row); i++) {
      digitalWrite(row[i], LOW);
      for (unsigned char j = 0; j < sizeof(column); j++) {
        *buttonStates |= (!digitalRead(column[j]) & 1UL) << buttonMatriceToDecimal(i, j);
  
        if (*buttonStates & buttonBitMask(i, j)) {
          *lastPressedButton = buttonMatriceToDecimal(i, j);
        }
      }
      digitalWrite(row[i], HIGH);
    }
  
    // Correct for hardware bug on last column (column 3)
    if (*buttonStates & buttonBitMask(0, 3)) {
      *buttonStates = *buttonStates & ~(buttonBitMask(0, 2) | buttonBitMask(0, 0));
    }
    if (*buttonStates & buttonBitMask(1, 3)) {
      *buttonStates = *buttonStates & ~(buttonBitMask(1, 2) | buttonBitMask(1, 0));
    }
    if (*buttonStates & buttonBitMask(2, 3)) {
      *buttonStates = *buttonStates & ~(buttonBitMask(2, 2) | buttonBitMask(2, 0));
    }
    if (*buttonStates & buttonBitMask(3, 3)) {
      *buttonStates = *buttonStates & ~(buttonBitMask(3, 2) | buttonBitMask(3, 0));
    }
    if (*buttonStates & buttonBitMask(4, 3)) {
      *buttonStates = *buttonStates & ~(buttonBitMask(4, 2) | buttonBitMask(4, 0));
    }

  } while (tempButtonStates != *buttonStates); // Compare so that the two runs have got the same value
}
