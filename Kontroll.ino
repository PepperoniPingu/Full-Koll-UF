#define NO_LED_FEEDBACK_CODE
#define RAW_BUFFER_LENGTH 120 // Number of bytes in saved in a raw recording. Should be at least 112. 
#define EXCLUDE_EXOTIC_PROTOCOLS

#define IR_RECEIVE_PIN PIN_PA7
#define IR_SEND_PIN PIN_PA4

#define SHORT_COLUMNS PIN_PB2

#define NUMBER_OF_REPEATS 2U
#define DELAY_BETWEEN_REPEAT 500
#define WAIT_BETWEEN_RECORDINGS 500 // If there are multiple recordings on a button, wait this amount of milliseconds betweeen sending out each recording. 

//#define DEBUG_PRINTING // Not enough memory for both serial and IRSender. Therefore only one can be used at a time. If this is defined, all will work except it won't send any codes. 
#define SERIAL_SPEED 115200

#include <IRremote.hpp>
#include <Wire.h>
#include "buttons.h"
#include "eeprom.h"

const unsigned char row[5] = {PIN_PB0, PIN_PB1, PIN_PA1, PIN_PA5, PIN_PA3};
const unsigned char column[4] = {PIN_PB3, PIN_PA2, PIN_PA4, PIN_PA6};
unsigned long buttonStates = 0UL;
unsigned long lastButtonStates = 0UL;
char lastPressedButton[2] = { -1, -1};

// This defines which buttons and in which order you need to press them in order to switch program mode. 
const unsigned char switchStateButtonSequence[] = {btn00ShiftLeft, btn10ShiftLeft, btn20ShiftLeft, btn30ShiftLeft, btn40ShiftLeft};
unsigned char progressInButtonSequence = 0;

// The different states the device can be in
enum ProgramState : unsigned char {
  remote,
  recording,
  stateCount
};

// The current state
ProgramState programState = remote;

unsigned long lastMillisButtonRecordingState = 0UL; // Only works in recording when the device doesn't go into sleeping
#define WAIT_AFTER_BUTTON 500

unsigned long buttonsToEdit = 0UL;  // When recording state is entered, all buttons previous recordings to will replaced with new recording. 
                                    // This variable is exists to see if the recording should replace previous made recording or if the packet should be edited. 

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

  #ifdef DEBUG_PRINTING
    Serial.swap(0); // Use serial interface 0
    Serial.begin(SERIAL_SPEED);
    Serial.println("In debug mode. Sending will not work. ");
  #endif
  
  // Starting tasks
  switch (programState) {
    case remote:

      // Somehow this is needed. If it is not here, the remote will only work after being in recording mode. Will investigate further. 
      IrReceiver.start();
      IrReceiver.decode();
      IrReceiver.end();
      millis();
      
      #ifdef DEBUG_PRINTING
        Serial.println("Starting as remote...");
      #endif
      break;

    case recording:
      IrReceiver.start();
      lastPressedButton[0] = -1;
      lastPressedButton[1] = -1;
      
      #ifdef DEBUG_PRINTING
        Serial.println("Starting in recording...");
      #endif
      break;

    default:
      break;
  }
}


void loop() {
  scanMatrix();
  
  // If a button has just been pressed
  if (buttonStates && buttonStates != lastButtonStates) {

    // Only used in recording state
    lastMillisButtonRecordingState = millis();
    
    // If the right button in the sequence to switch program state is pressed, move on to check for the next button in the sequence
    if (buttonStates & (1UL << switchStateButtonSequence[0])) {
      progressInButtonSequence = 1;
    }
    else if (buttonStates & (1UL << switchStateButtonSequence[progressInButtonSequence])) {
      progressInButtonSequence++;
      
    } else { // If a button has just been pressed but it is not the next one in the  right sequence
      progressInButtonSequence = 0;
    }
  }

  #ifdef DEBUG_PRINTING
    if (buttonStates && buttonStates != lastButtonStates) {
      Serial.print("Progress in sequence to switch program state: ");
      Serial.print(progressInButtonSequence, DEC);
      Serial.print("/"),
      Serial.println(sizeof(switchStateButtonSequence), DEC);
    }
  #endif

  // If the last button of the state switching sequence has been pressed, change program state. 
  if (progressInButtonSequence >= sizeof(switchStateButtonSequence)) {
    progressInButtonSequence = 0; // Reset progress in sequence

    // Shutdown tasks
    switch (programState) {
      case remote:
        break;

      case recording:
        IrReceiver.stop();
        lastPressedButton[0] = -1;
        lastPressedButton[1] = -1;
        buttonsToEdit = 0UL;
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
        #ifdef DEBUG_PRINTING
          Serial.println("Switching to remote...");
        #endif
        break;

      case recording:
        IrReceiver.start();
        lastPressedButton[0] = -1;
        lastPressedButton[1] = -1;
        buttonsToEdit = 0UL;
        
        #ifdef DEBUG_PRINTING
          Serial.println("Switching to recording...");
        #endif
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

      // If a button is pressed
      if (buttonStates & buttonBitMask(i, j)) {
        Wire.swap(0);
        Wire.usePullups();
        Wire.begin();
         
        unsigned char recordingsOnButton = readRecordingsOnButton(i, j);
        
        #ifdef DEBUG_PRINTING
          Serial.print("Button pressed ");
          Serial.print(i, DEC);
          Serial.println(j, DEC);
          Serial.print("Bit mask: ");
          Serial.println(buttonStates, BIN);
          Serial.print("Recordings: ");
          Serial.println(recordingsOnButton, DEC);
        #endif
        
        if (recordingsOnButton) {
          IRData buttonPacket[recordingsOnButton];
          readButtonPacket(buttonPacket, i, j); // Read button packet
  
            // Loop through every recording and send it
            for (unsigned char k = 0; k < recordingsOnButton; k++) {
              if (lastButtonStates == buttonStates) {
                  buttonPacket[k].flags = IRDATA_FLAGS_IS_REPEAT;
              }
              
              #ifndef DEBUG_PRINTING // Not enough memory for both serial and IRSender
                IrSender.write(&buttonPacket[k], NUMBER_OF_REPEATS); // Send recording
              #endif

              #ifdef DEBUG_PRINTING
                Serial.print("Protocol: ");
                Serial.print(buttonPacket[k].protocol, DEC); 
                Serial.print(" Command: ");
                Serial.println(buttonPacket[k].command, DEC);
              #endif 
              
              if (recordingsOnButton > 1 && k < recordingsOnButton - 1) {
                delay(WAIT_BETWEEN_RECORDINGS); // Wait before sending next one
              }
            }
            
            delay(DELAY_BETWEEN_REPEAT); // Wait a bit between retransmissions
        }

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
    #ifdef DEBUG_PRINTING
      Serial.println("Sleep initiating...\n");
    #endif
    
    sleep();
    wakeProcedure();
    
    #ifdef DEBUG_PRINTING
      Serial.println("Waking up...");
    #endif
  }
}


// TODO: Make it possible to store multiple recordings on one button 
void recordingProgram() {
  #ifdef DEBUG_PRINTING
    for (unsigned char i = 0; i < sizeof(row); i++) {
      for (unsigned char j = 0; j < sizeof(column); j++) {
        if ((buttonStates & buttonBitMask(i, j)) && !(lastButtonStates & buttonBitMask(i, j))) {
          Serial.print("\nButton pressed ");
          Serial.print(i, DEC);
          Serial.println(j, DEC);
        }
      }
    }
  #endif 

  // If there is recieved data and a button was pressed, decode. Only record when all buttons have been released for WAIT_AFTER_BUTTON since buttons on column 3 and 4 activate the sendere and that garbage can get recieved.
  if (IrReceiver.decode() && (lastPressedButton[0] != -1) && (lastPressedButton[1] != -1) && buttonStates == 0 && millis() - lastMillisButtonRecordingState > WAIT_AFTER_BUTTON) {
    Wire.swap(0);
    Wire.usePullups();
    Wire.begin();

    unsigned char numberOfRecordings = 1;
    if (buttonsToEdit & buttonBitMask(lastPressedButton[0], lastPressedButton[1])) {
      numberOfRecordings = readRecordingsOnButton(lastPressedButton[0], lastPressedButton[1]) + 1;  

      #ifdef DEBUG_PRINTING
        Serial.println("Adding to button packet");
      #endif
    } else {
      buttonsToEdit |= buttonBitMask(lastPressedButton[0], lastPressedButton[1]);

      #ifdef DEBUG_PRINTING
        Serial.println("Creating new button packet. Discarding old recordings on button. ");
      #endif
    }
    IRData buttonRecordings[numberOfRecordings];

    if (numberOfRecordings > 1) {
      readButtonPacket(buttonRecordings, lastPressedButton[0], lastPressedButton[1]); // Read the existing button packet
    }
    buttonRecordings[numberOfRecordings - 1] = {*IrReceiver.read()}; // Decode recieved data and add it to the existing packet
    buttonRecordings[numberOfRecordings - 1].flags = 0; // clear flags -esp. repeat- for later sending
    
    // Mark old packet as non-existing
    writeEEPROM(buttonInfoAddress(lastPressedButton[0], lastPressedButton[1]), 0);
    writeEEPROM(buttonInfoAddress(lastPressedButton[0], lastPressedButton[1]) + 1, 0);
    // Write the updated packet
    writeButtonPacket(buttonRecordings, numberOfRecordings, lastPressedButton[0], lastPressedButton[1]);
    Wire.end();
    pinMode(PIN_PB0, OUTPUT);
    digitalWrite(PIN_PB0, HIGH);
    pinMode(PIN_PB1, OUTPUT);
    digitalWrite(PIN_PB1, HIGH);

    #ifdef DEBUG_PRINTING
      Serial.print("IRResults: ");
      IrReceiver.printIRResultMinimal(&Serial);
      Serial.print("\nRecording saved on button  ");
      Serial.print(lastPressedButton[0], DEC);
      Serial.println(lastPressedButton[1], DEC);
      Serial.println();
    #endif

    lastPressedButton[0] = -1;
    lastPressedButton[1] = -1;

    IrReceiver.resume();
  }

  while (IrReceiver.decode()) {
    IrReceiver.resume();
    
    #ifdef DEBUG_PRINTING
      Serial.print("Flushing: ");
      IrReceiver.printIRResultMinimal(&Serial);
      Serial.println();
    #endif
  } 
}


// Function for reading the buttons
void scanMatrix() {
  #ifdef DEBUG_PRINTING
    Serial.end(); // Serial doesn't work when SHORT_COLUMNS is high. Therefore, to scan the button matrix, serial has to be disabled. 
  #endif
  
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

  #ifdef DEBUG_PRINTING
    Serial.begin(SERIAL_SPEED); // Turn serial back on once the button scanning is done
  #endif
}


// Procedure to prepare for sleep and start sleeping
void sleep() {
  #ifdef DEBUG_PRINTING
    Serial.end(); // Because serial TX and SHORT_COLUMNS is the same pin serial has to be disabled. 
  #endif
  
  Wire.end(); // Should not be needed but if it is not done before sleeping, some of the buttons will not work for waking. 
  
  pinMode(IR_SEND_PIN, INPUT_PULLUP); // IR_SEND_PIN is normally high. Since it's the same pin as column 2, it will short column 0, 2 and 3 to permanently high.
                                      // That will not work to generate a falling interrupt. Therefore it needs to be an input. 

  pinMode(IR_RECEIVE_PIN, INPUT_PULLUP); // Leaving it floating will draw current

  // Set all rows to low so a falling interrupt can be generated when button is pressed 
  for (unsigned char i = 0; i < sizeof(row); i++) {
    pinMode(row[i], OUTPUT);
    digitalWrite(row[i], LOW);
  }
  
  // Short column 0, 2 and 3 so that they all connect to PIN_PA6 and all buttons can generate interrupts. 
  pinMode(SHORT_COLUMNS, OUTPUT);
  digitalWrite(SHORT_COLUMNS, LOW); // Active low

  // Set every column as input so that it is not interfering 
  for (unsigned char i = 0; i < sizeof(column); i++) {
    pinMode(column[i], INPUT_PULLUP);
  }

  RTC.PITCTRLA &= 0b11111110; // Disable PIT

  /*PORTA.PIN0CTRL = 0b00001100; // UPDI, pullup enabled, interrupt and buffer disabled
  PORTA.PIN1CTRL = 0b00000100; // Row 3, pullup, interrupt and buffer disabled
  PORTA.PIN3CTRL = 0b00000100; // Row 5, pullup, interrupt and buffer disabled
  PORTA.PIN4CTRL = 0b00001100; // Column 3 and IR LED, pullup enabled, interrupt and buffer disabled
  PORTA.PIN5CTRL = 0b00000100; // Row 4, pullup, interrupt and buffer disabled
  PORTA.PIN7CTRL = 0b00001100; // IR Reciever, pullup enabled, interrupt and buffer disabled
  PORTB.PIN0CTRL = 0b00000100; // Row 1, pullup, interrupt and buffer disabled
  PORTB.PIN1CTRL = 0b00000100; // Row 2, pullup, interrupt and buffer disabled
  PORTB.PIN2CTRL = 0b00000100; // SHORT_COLUMNS, pullup, interrupt and buffer disabled 
  PORTB.PIN3CTRL = 0b00001100; // Column 1, pullup enabled, interrupt and buffer disabled*/
  
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

  #ifdef DEBUG_PRINTING
    Serial.begin(SERIAL_SPEED);  // Start serial again
  #endif
}


// Each button has a permanent address for info. The info contains another address to where a "button packet" is found. A button packet contains 
// number of signals, protocol type and protocol data are stored. The info is 2 bytes which are the address of the button packet. 
// This function returns the address for the permanent address for the button info. 
unsigned int buttonInfoAddress(unsigned char tempRow, unsigned char tempColumn) {
  unsigned int tempInfoAddress = 2 * buttonShiftLeft[tempRow][tempColumn];
  return tempInfoAddress;
}


// Function to return the dynamic address of a button packet. 
unsigned int buttonPacketAddress(unsigned char tempRow, unsigned char tempColumn) {

  // Read the adress of the button packet
  unsigned int tempButtonPacketAddress = 0;
  tempButtonPacketAddress |= readEEPROM(buttonInfoAddress(tempRow, tempColumn)) << 8;
  tempButtonPacketAddress |= readEEPROM(buttonInfoAddress(tempRow, tempColumn) + 1);

  // Reset the info packet and return 0 if one of following conditions is true: 
  if ((tempButtonPacketAddress > MAX_EEPROM_ADDRESS - sizeof(IRData) - 1) ||                                                        // The addres to the buttonPacket is bigger than available memory
        (tempButtonPacketAddress <= (buttonInfoAddress(sizeof(row) - 1, sizeof(column) - 1) + 1) && tempButtonPacketAddress != 0) ||  // The address is in the button info region (first 40 bytes)
        (readEEPROM(tempButtonPacketAddress) == 0)) {                                                                                  // There are no recordings here and therefore the packet is incomplete
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
    
    unsigned char tempNumberOfRecordings = readRecordingsOnButton(tempRow, tempColumn); // Read the number of recordings
    //Serial.print("NumberOfRecordings ");
    //Serial.println(tempNumberOfRecordings, DEC);
    // Only return button packets if there are any
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF) {
      unsigned char tempRecordings[tempNumberOfRecordings][sizeof(IRData)];

      // Loop through every recording
      for (unsigned char i = 0; i < tempNumberOfRecordings; i++) {
        //Serial.println("New packet");
        // Loop through every byte in said recording and save it
        for (unsigned char j = 0; j < sizeof(IRData); j++) {
          tempRecordings[i][j] = readEEPROM(tempButtonPacketAddress + sizeof(IRData) * i + 1 + j);
          /*Serial.print("Recording: ");
          Serial.print(i, DEC);
          Serial.print(" Byte number: ");
          Serial.print(j, DEC);
          Serial.print(" Address: ");
          Serial.print(tempButtonPacketAddress + sizeof(IRData) * i + 1 + j);
          Serial.print(" Byte: ");
          Serial.println(tempRecordings[i][j], DEC);*/
        }
      } 
      memcpy(buttonPacketPtr, &tempRecordings, sizeof(IRData) * tempNumberOfRecordings);   // Return the recordings to the pointer given in the arguments 
      return tempNumberOfRecordings;                                                       // Return number of recordings
      
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


// Returns number of recordings in a packet
unsigned char readRecordingsOnButton(unsigned char tempRow, unsigned char tempColumn) {
  unsigned int tempButtonPacketAddress = buttonPacketAddress(tempRow, tempColumn); // Find the address of the button packet

  // If the packet address is 0 then there is no packet and return 0
  if (tempButtonPacketAddress != 0) {
    unsigned char tempNumberOfRecordings = readEEPROM(tempButtonPacketAddress); // Read the number of recordings
    
    // Take action if there are 0 recordings 
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF) {
        return tempNumberOfRecordings; // Return packet length
        
    } else {
      // If there are 0 recordings here, reset the info data
      unsigned int tempButtonInfoAddress = buttonInfoAddress(tempRow, tempColumn);
      writeEEPROM(tempButtonInfoAddress, 0);
      writeEEPROM(tempButtonInfoAddress + 1, 0);
    }
  }
  
  return 0;
}


// Returns total length of button packet
unsigned char readButtonPacketLength(unsigned char tempRow, unsigned char tempColumn) {
  unsigned char tempRecordings = readRecordingsOnButton(tempRow, tempColumn);
  if (tempRecordings != 0) {
    return tempRecordings * sizeof(IRData) + 1;
  }
  return 0;
}


// Writes a button packet
// TODO: add error handling if given tempAddress is not allowed, for example 0
void writeButtonPacket(IRData tempIRData[], unsigned char recordings, unsigned char tempRow, unsigned char tempColumn) {

  unsigned int tempAddress = scanEmptyEEPROMAddresses(sizeof(IRData) * recordings + 1); // Find an empty address space where the packet can be put

  #ifdef DEBUG_PRINTING
    Serial.print("Writing ");
    Serial.print(recordings, DEC);
    Serial.print(" recordings to address ");
    Serial.println(tempAddress, DEC);
  #endif
  
  // Update button info. Change the address to the new found one.
  writeEEPROM(buttonInfoAddress(tempRow, tempColumn), tempAddress >> 8);  
  writeEEPROM(buttonInfoAddress(tempRow, tempColumn) + 1, tempAddress); 

  // Remake the array of IRData to array of bytes
  unsigned char tempRawData[sizeof(IRData) * recordings];
  memcpy(&tempRawData, tempIRData, sizeof(IRData) * recordings);

  writeEEPROM(tempAddress, recordings); // First byte is number of recordings saved
  // Write the other bytes
  for (unsigned int i = 0; i < sizeof(tempRawData); i++) {
    writeEEPROM(tempAddress + i + 1, tempRawData[i]);

    #ifdef DEBUG_PRINTING
      Serial.print("Writing ");
      Serial.print(tempRawData[i], DEC);
      Serial.print(" to address ");
      Serial.println(tempAddress + 1 + i, DEC);
    #endif
  }
  #ifdef DEBUG_PRINTING
    Serial.println();
  #endif
}


// Find the smallest address space where the bytes can fit
// TODO: Make it so that spaces can't be nagative and so that they can't overlap
unsigned int scanEmptyEEPROMAddresses(unsigned int bytesRequired) {
  const unsigned char tempNumberOfButtons = sizeof(row) * sizeof(column);
  unsigned int packetAddresses[tempNumberOfButtons + 2];
  unsigned int packetLengths[tempNumberOfButtons + 2];

  // Retrieve addresses and lengths of all packets
  for (unsigned char i = 0; i < tempNumberOfButtons; i++) {
    unsigned char tempRow;
    unsigned char tempColumn;
    buttonDecimalToMatrice(&tempRow, &tempColumn, i);               // Remake button 0 to 19 into row and column
    packetAddresses[i] = buttonPacketAddress(tempRow, tempColumn);  // Retreive address of packet
    packetLengths[i] = readButtonPacketLength(tempRow, tempColumn); // Retrieve length of packet
  }
  // Make an entry for the sapce occupied by the button info data
  packetAddresses[tempNumberOfButtons] = 0;
  packetLengths[tempNumberOfButtons] = buttonInfoAddress(sizeof(row) - 1, sizeof(column) - 1) + 2;
  // Make an entry the end of the chip
  packetAddresses[tempNumberOfButtons + 1] = MAX_EEPROM_ADDRESS;
  packetLengths[tempNumberOfButtons + 1] = 0;

  // Sort addresses and lengths, small to large
  for (unsigned char i = 0; i < tempNumberOfButtons; i++) {
    for (unsigned char j = i + 1; j < tempNumberOfButtons + 1; j++) {
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

  #ifdef DEBUG_PRINTING
    // Prints empty address spaces
    Serial.print("Searching for empty address space that fits ");
    Serial.print(bytesRequired, DEC);
    Serial.println(" bytes...");
    Serial.println("Available spaces: ");
    for (unsigned char i = 0; i < tempNumberOfButtons + 1; i++) {
      Serial.print(i);
      Serial.print(" : ");
      Serial.print(packetAddresses[i] + packetLengths[i], DEC);
      Serial.print(" -> ");
      Serial.println(packetAddresses[i + 1], DEC);
    }
    Serial.println();
  #endif

  // Find the smallest space where the bytes will fit and return that address
  unsigned int shortestSpaceAddress = 0xFFFF;
  unsigned int addressSpace = 0;
  unsigned int bestAddress = 0;
  // Iterate through every button packet
  for (unsigned char i = 0; i < tempNumberOfButtons + 1; i++) {
    addressSpace = packetAddresses[i+1] - (packetAddresses[i] + packetLengths[i]); // Save the address of the empty space
    
    // Compare the empty space, if it is large enough but also the smallest one discover yet, save it. 
    if (addressSpace >= bytesRequired && addressSpace < shortestSpaceAddress) {
      shortestSpaceAddress = addressSpace;
      bestAddress = packetAddresses[i] + packetLengths[i];
    }
  }
  
  return bestAddress;
}
