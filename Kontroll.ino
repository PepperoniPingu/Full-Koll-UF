//#define EXCLUDE_EXOTIC_PROTOCOLS enabled
#define NO_LED_FEEDBACK_CODE enabled
#define RAW_BUFFER_LENGTH 120

#define IR_RECEIVE_PIN PIN_PA7
#define IR_SEND_PIN PIN_PA4

#define SHORT_COLUMNS PIN_PB2

#define NUMBER_OF_REPEATS 3

#define SERIAL_SPEED 115200

#include <IRremote.hpp>
#include <Wire.h>
#include "buttons.h"
#include "eeprom.h"

#define ROWS 5
#define COLUMNS
const unsigned char row[ROWS] = {PIN_PB0, PIN_PB1, PIN_PA1, PIN_PA5, PIN_PA3};
const unsigned char column[COLUMNS] = {PIN_PB3, PIN_PA2, PIN_PA4, PIN_PA6};

unsigned long buttonStates = 0UL;
unsigned long lastButtonStates = 0UL;
char lastPressedButton[2] = { -1, -1};

// The different states the device can be in
enum ProgramState : unsigned char {
  remote,
  recording,
  stateCount
};

// The current state
ProgramState programState = remote;


void setup() {

  for (unsigned char i = 0; i < sizeof(row); i++) {
    pinMode(row[i], OUTPUT);
    digitalWrite(row[i], HIGH);
  }

  for (unsigned char i = 0; i < sizeof(column); i++) {
    pinMode(column[i], INPUT_PULLUP);
  }

  pinMode(IR_SEND_PIN, OUTPUT);
  digitalWrite(IR_SEND_PIN, HIGH);

  pinMode(SHORT_COLUMNS, OUTPUT);
  digitalWrite(SHORT_COLUMNS, HIGH);

  IrReceiver.begin(IR_RECEIVE_PIN);
  
  IrSender.begin(IR_SEND_PIN);
  IrSender.enableIROut(38);
  PORTA.PIN4CTRL |= PORT_INVEN_bm;

  Serial.swap(0); // Use serial interface 0

  Serial.begin(SERIAL_SPEED);
  
  // Starting tasks
  switch (programState) {
    case remote:
      Serial.println("Starting as remote");
      break;

    case recording:
      Serial.print("Starting in recording");
      IrReceiver.start();
      lastPressedButton[0] = -1;
      lastPressedButton[1] = -1;
      break;

    default:
      break;
  }
}

void loop() {
   scanMatrix();

  // Switch program state when btn41 is pressed
  if ((buttonStates & buttonBitMask(4, 1)) && !(lastButtonStates & buttonBitMask(4, 1))) {
    // Shutdown tasks
    switch (programState) {
      case remote:
        break;

      case recording:
        IrReceiver.stop();
        lastPressedButton[0] = -1;
        lastPressedButton[1] = -1;
        break;

      default:
        break;
    }

    // Switch state
    if (programState >= stateCount - 1) {
      programState = ProgramState(0);
    } else {
      programState = ProgramState(int(programState) + 1);
    }

    // Starting tasks
    switch (programState) {
      case remote:
        Serial.println("Switching to remote");
        break;

      case recording:
        Serial.print("Switching to recording");
        IrReceiver.start();
        lastPressedButton[0] = -1;
        lastPressedButton[1] = -1;
        break;

      default:
        break;
    }
  }


  // Chose program routine
  switch (programState) {
    case remote:
      remoteProgram();
      break;

    case recording:
      recordingProgram();
      break;

    default:
      remoteProgram();
      break;
  }
}


void remoteProgram() {

  for (unsigned char i = 0; i < sizeof(row); i++) {
    for (unsigned char j = 0; j < sizeof(column); j++) {

      if ((buttonStates & buttonBitMask(i, j)) && !(lastButtonStates & buttonBitMask(i, j))) {
        Serial.print("Knapp tryckt ");
        Serial.print(i, DEC);
        Serial.println(j, DEC);
        Serial.print("Bit mask: ");
        Serial.println(buttonStates, BIN);

        Wire.begin();
        Serial.print("Empty address: ");
        Serial.println(scanEmptyEEPROMAddresses(178), HEX);
        unsigned char recordingsOnButton = readEEPROM(buttonPacketAddress(i, j));
        if (recordingsOnButton == 0xFF) {
          writeEEPROM(buttonInfoAddress(i, j), 0);
          writeEEPROM(buttonInfoAddress(i, j) + 1, 0);
          writeEEPROM(buttonPacketAddress(i, j), 0);
          writeEEPROM(buttonPacketAddress(i, j) + 1, 0);
          recordingsOnButton = 0;
        }
        Serial.print("Recordings: ");
        Serial.println(recordingsOnButton, HEX);
        IRData buttonPacket[recordingsOnButton];
        for (unsigned char k = 0; k < recordingsOnButton; k++) {
          readButtonPacket(buttonPacket, i, j);
        }
        //Serial.println(buttonPacket[0].protocol);
        Wire.end();
        pinMode(PIN_PB0, OUTPUT);
        digitalWrite(PIN_PB0, HIGH);
        pinMode(PIN_PB1, OUTPUT);
        digitalWrite(PIN_PB1, HIGH);
        
      }
    }
  }

  // Only sleep if no buttons are pressed
  if (buttonStates == 0UL) {
    Serial.println("Somnar");
    sleep();
    wakeProcedure();
    Serial.println("Vaknar");
  }
}


void recordingProgram() {
  for (unsigned char i = 0; i < sizeof(row); i++) {
    for (unsigned char j = 0; j < sizeof(column); j++) {
      if ((buttonStates & buttonBitMask(i, j)) && !(lastButtonStates & buttonBitMask(i, j))) {
        Serial.print("Button pressed ");
        Serial.print(i, DEC);
        Serial.println(j, DEC);
      }
    }
  }

  // If there is recieved data and a button was pressed, decode
  if (IrReceiver.decode() && (lastPressedButton[0] != -1) && (lastPressedButton[1] != -1)) {
      //IrReceiver.printIRResultMinimal(&Serial);
      Serial.println();
      Wire.begin();
      IRData recievedIRData[1] = {*IrReceiver.read()};
      writeButtonPacket(recievedIRData, 1, lastPressedButton[0], lastPressedButton[1]);
      Wire.end();
      pinMode(PIN_PB0, OUTPUT);
      digitalWrite(PIN_PB0, HIGH);
      pinMode(PIN_PB1, OUTPUT);
      digitalWrite(PIN_PB1, HIGH);
      Serial.print("kod sparad på knapp ");
      Serial.print(lastPressedButton[0], DEC);
      Serial.println(lastPressedButton[1], DEC);

      lastPressedButton[0] = -1;
      lastPressedButton[1] = -1;

      IrReceiver.resume();

      while (IrReceiver.decode()) {
        IrReceiver.printIRResultMinimal(&Serial);
        Serial.println();
        IrReceiver.resume();
      }
    }
}


// Function for reading the buttons
void scanMatrix() {
  // Serial doesn't work when SHORT_COLUMNS is high. Therefore, to scan the button matrix, serial has to be disabled. 
  Serial.end();
  // SHORT_COLUMS is active low and needs to be disabled to read individual button presses. 
  pinMode(SHORT_COLUMNS, OUTPUT); 
  digitalWrite(SHORT_COLUMNS, HIGH); // Active low
  
  // IR_SEND_PIN is used for both button matrix and sending IR. When sending IR-codes it needs to be inverted since the IR LED is active low. 
  pinMode(IR_SEND_PIN, INPUT_PULLUP);
  PORTA.PIN4CTRL &= ~PORT_INVEN_bm;
  
  // Copy last button state
  lastButtonStates = buttonStates;

  unsigned long tempButtonStates;

  // Loop atleast two times and make sure the buttons have the same states both times. Needed because of hardware bug that can make it register wrong button presses if last 
  // column of buttons is pressed in the middle of the scanning process.
  do {
    tempButtonStates = buttonStates;
  
    // Reset button states
    buttonStates = 0;
    
    // Scan matrix and update array
    for (unsigned char i = 0; i < sizeof(row); i++) {
      digitalWrite(row[i], LOW);
      for (unsigned char j = 0; j < sizeof(column); j++) {
        buttonStates |= (!digitalRead(column[j]) & 1UL) << buttonShiftLeft[i][j];
  
        if (buttonStates & buttonBitMask(i, j)) {
          lastPressedButton[0] = i;
          lastPressedButton[1] = j;
        }
      }
      digitalWrite(row[i], HIGH);
    }
  
    // Correct for hardware bug on last column (column 3)
    if (buttonStates & buttonBitMask(0, 3)) {
      buttonStates = buttonStates & ~(buttonBitMask(0, 2) | buttonBitMask(0, 0));
    }
    if (buttonStates & buttonBitMask(1, 3)) {
      buttonStates = buttonStates & ~(buttonBitMask(1, 2) | buttonBitMask(1, 0));
    }
    if (buttonStates & buttonBitMask(2, 3)) {
      buttonStates = buttonStates & ~(buttonBitMask(2, 2) | buttonBitMask(2, 0));
    }
    if (buttonStates & buttonBitMask(3, 3)) {
      buttonStates = buttonStates & ~(buttonBitMask(3, 2) | buttonBitMask(3, 0));
    }
    if (buttonStates & buttonBitMask(4, 3)) {
      buttonStates = buttonStates & ~(buttonBitMask(4, 2) | buttonBitMask(4, 0));
    }

  } while (tempButtonStates != buttonStates); // Compare so that the two runs have got the same value

  // Revert IR_SEND_PIN so it can be used for sending IR
  PORTA.PIN4CTRL |= PORT_INVEN_bm;
  pinMode(IR_SEND_PIN, OUTPUT);
  digitalWrite(IR_SEND_PIN, LOW);

  Serial.begin(SERIAL_SPEED); // Turn serial back on once the button scanning is done
}


// Procedure to prepare for sleep and start sleeping
void sleep() {
  pinMode(IR_SEND_PIN, INPUT_PULLUP); // IR_SEND_PIN is normally high. Since it's the same pin as column 2, it will short column 0, 2 and 3 to permanently high.
                                      // That will not work to generate a falling interrupt. Therefore it needs to be an input. 

  // Set all rows to low so a falling interrupt can be generated when button is pressed 
  for (unsigned char i = 0; i < sizeof(row); i++) {
    digitalWrite(row[i], LOW);
  }

  Serial.end(); // Because serial TX and SHORT_COLUMNS is the same pin serial has to be disabled. 
  // Short column 0, 2 and 3 so that they all connect to PIN_PA6 and all buttons can generate interrupts. 
  pinMode(SHORT_COLUMNS, OUTPUT);
  digitalWrite(SHORT_COLUMNS, LOW); // Active low
  
  PORTA.PIN2CTRL = 0b00001011; // Pull up enabled and interrupt on falling edge configured for PIN_PA2
  PORTA.PIN6CTRL = 0b00001011; // Same thing for PIN_PA6

  SLPCTRL.CTRLA = 0b00000101; // Set sleep mode to power-down and enable sleeping
  __asm__ __volatile__ ( "sleep" "\n\t" :: ); // Start sleeping
}

// Interrupt for when a button is pressed to wake the cpu
ISR(PORTA_PORT_vect) {
  byte flags = PORTA.INTFLAGS;
  PORTA.INTFLAGS = flags; //clear flags
  
  // Disable interrupt on PIN_PA2 and PIN_PA6. Needed in case the button bounces and we don't want to trigger the interrupt when it's already running. Pull ups still enabled.
  PORTA.PIN2CTRL = 0b00001000;
  PORTA.PIN6CTRL = 0b00001000;
}

// Procedure when waking up to set things back to normal and enable peripherals
void wakeProcedure() {  
  SLPCTRL.CTRLA = 0b00000100; // Disable sleeping

  // Revert rows to high
  for (unsigned char i = 0; i < sizeof(row); i++) {
    digitalWrite(row[i], HIGH);
  }

  // Revert IR_SEND_PIN so it can be used for sending IR
  pinMode(IR_SEND_PIN, OUTPUT);
  digitalWrite(IR_SEND_PIN, HIGH);

  Serial.begin(SERIAL_SPEED);  // Start serial again
}


// Each button has a permanent address for info. The info contains another address to where a "button packet" is found. A button packet contains 
// number of signals, protocol type and protocol data are stored. The info is 2 bytes which are the address of the button packet. 
// This function returns the address for the permanent address for the button info. 
unsigned int buttonInfoAddress(unsigned char tempRow, unsigned char tempColumn) {
  return 2 * buttonShiftLeft[tempRow][tempColumn];
}


// Function to return the dynamic address of a button packet. 
unsigned int buttonPacketAddress(unsigned char tempRow, unsigned char tempColumn) {

  // Read the adress of the button packet
  unsigned int tempButtonPacketAddress;
  tempButtonPacketAddress = readEEPROM(buttonInfoAddress(tempRow, tempColumn)) << 8;
  tempButtonPacketAddress |= readEEPROM(buttonInfoAddress(tempRow, tempColumn) + 1);

  // If the addres to the buttonPacket is bigger than available memory or it is in the button info region (first 40 bytes), reset the info packet and return 0. 
  if (tempButtonPacketAddress > MAX_EEPROM_ADDRESS - sizeof(IRData) - 1 || (tempButtonPacketAddress <= (buttonInfoAddress(ROWS - 1, COLUMNS - 1) + 1) && tempButtonPacketAddress != 0)) {
    writeEEPROM(buttonInfoAddress(tempRow, tempColumn), 0);
    writeEEPROM(buttonInfoAddress(tempRow, tempColumn) + 1, 0);
    tempButtonPacketAddress = 0;
  }
  
  return tempButtonPacketAddress;
}


// First byte of the button packet contains the number of recordings
// Second byte and after contains the first recording's IRData
unsigned char readButtonPacket(IRData buttonPacketPtr[], unsigned char tempRow, unsigned char tempColumn) {

  unsigned int tempButtonPacketAddress = buttonPacketAddress(tempRow, tempColumn); // Find the address of the button packet

  // If the packet address is 0 then there is no packet and return 0
  if (tempButtonPacketAddress != 0) {
    
    unsigned char tempNumberOfRecordings = readEEPROM(tempButtonPacketAddress); // Read the number of recordings

    // Only return button packets if there are any
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF) {
      unsigned char tempRecordings[tempNumberOfRecordings][sizeof(IRData)];
      
      // Loop through every recording
      for (unsigned char i = 0; i < tempNumberOfRecordings; i++) {
        
        // Loop through every byte in said recording and save it
        for (unsigned char j = 0; j < sizeof(IRData); j++) {
          tempRecordings[i][j] = readEEPROM(tempButtonPacketAddress + 1 + j);
        }
      } 
      memcpy(buttonPacketPtr, &tempRecordings, sizeof(IRData));  // Return the recordings to the pointer given in the arguments 
      return tempNumberOfRecordings;                              // Return number of recordings
      
    } else {
      // If there are 0 recordings here, reset the info data
      unsigned int tempButtonInfoAddress = buttonInfoAddress(tempRow, tempColumn);
      writeEEPROM(tempButtonInfoAddress, 0);
      writeEEPROM(tempButtonInfoAddress + 1, 0);
    }
  }

  // If earlier return didn't complete, return o. 
  return 0;
}


// Returns total length of button packet
unsigned char readButtonPacketLength(unsigned char tempRow, unsigned char tempColumn) {
  unsigned int tempButtonPacketAddress = buttonPacketAddress(tempRow, tempColumn); // Find the address of the button packet

  // If the packet address is 0 then there is no packet and return 0
  if (tempButtonPacketAddress != 0) {
    
    unsigned char tempNumberOfRecordings = readEEPROM(tempButtonPacketAddress); // Read the number of recordings

    // Take action if there are 0 recordings 
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF) {
        return tempNumberOfRecordings; // Return number of recordings
    } else {
      // If there are 0 recordings here, reset the info data
      unsigned int tempButtonInfoAddress = buttonInfoAddress(tempRow, tempColumn);
      writeEEPROM(tempButtonInfoAddress, 0);
      writeEEPROM(tempButtonInfoAddress + 1, 0);
    }
  }
  return 0;
}


// Writes a button packet
void writeButtonPacket(IRData tempIRData[], unsigned char recordings, unsigned char tempRow, unsigned char tempColumn) {

  unsigned int tempAddress = scanEmptyEEPROMAddresses(sizeof(IRData) * recordings + 1); // Find an empty address space where the packet can be put
  // TODO: add error handling if given tempAddress is not allowed, for example 0
  
  // Update button info. Change the address to the new found one.
  writeEEPROM(buttonInfoAddress(tempRow, tempColumn), tempAddress >> 8);  
  writeEEPROM(buttonInfoAddress(tempRow, tempColumn) + 1, tempAddress); 

  // Remake the array of IRData to array of bytes
  unsigned char tempRawData[sizeof(IRData)];
  memcpy(&tempRawData, tempIRData, sizeof(IRData));

  writeEEPROM(tempAddress, recordings); // First byte is number of recordings saved
  // Write the other bytes
  for (unsigned int i = 0; i < sizeof(IRData); i++) {
    writeEEPROM(tempAddress + i + 1, tempRawData[i]);
  }
}


// Find the smallest address space where the bytes can fit
unsigned int scanEmptyEEPROMAddresses(unsigned int bytesRequired) {
  const unsigned char tempNumberOfButtons = sizeof(row) * sizeof(column);
  unsigned int packetAddresses[tempNumberOfButtons + 2];
  unsigned int packetLengths[tempNumberOfButtons + 2];

  // Retrieve addresses and lengths of all packets
  for (unsigned char i = 0; i < tempNumberOfButtons - 2; i++) {
    unsigned char tempRow;
    unsigned char tempColumn;
    buttonDecimalToMatrice(&tempRow, &tempColumn, i); // Remake button 0 to 19 into row and column
    packetAddresses[i] = buttonPacketAddress(tempRow, tempColumn); // Retreive address of packet
    packetLengths[i] = readButtonPacketLength(tempRow, tempColumn); // Retrieve length of packet
  }
  // Make an entry for the sapce occupied by the button indo data
  packetAddresses[tempNumberOfButtons + 1] = 0;
  packetLengths[tempNumberOfButtons + 1] = buttonInfoAddress(ROWS - 1, COLUMNS - 1) + 1;
  // Make an entry the end of the chip
  packetAddresses[tempNumberOfButtons + 2] = MAX_EEPROM_ADDRESS;
  packetLengths[tempNumberOfButtons + 2] = 0;

  // Sort addresses and lengths, small to large
  for (unsigned char i = 0; i < tempNumberOfButtons - 1; i++) {
    for (unsigned char j = i + 1; j < tempNumberOfButtons; j++) {
      if (packetAddresses[i] > packetAddresses[j]) {
        // Swapping with smallest element of array
        unsigned int temp = packetAddresses[j];
        packetAddresses[j] = packetAddresses[i];
        packetAddresses[i] = temp;
        temp = packetLengths[j];
        packetLengths[j] = packetLengths[i];
        packetLengths[i] = temp;
      }
    }
  }
  // Debug
  for (unsigned char i = 0; i < tempNumberOfButtons; i++) {
    Serial.println(packetAddresses[i], HEX);
  }

  // Find the smallest space where the bytes will fit and return that address
  unsigned int shortestSpaceAddress = 0xFFFF;
  unsigned int addressSpace = 0;
  unsigned int bestAddress = 0;
  // Iterate through every button packet
  for (unsigned char i = 0; i < tempNumberOfButtons; i++) {
    addressSpace = packetAddresses[i+1] - (packetAddresses[i] + packetLengths[i]); // Save the address of the empty space
    
    // Compare the empty space, if it is large enough but also the smallest one discover yet, save it. 
    if (addressSpace <= bytesRequired && addressSpace < shortestSpaceAddress) {
      shortestSpaceAddress = addressSpace;
      bestAddress = packetAddresses[i] + packetLengths[i];
    }
  }
  
  return bestAddress;
}
