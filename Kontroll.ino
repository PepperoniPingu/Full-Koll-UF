#include "options.h"
#include <IRremote.hpp>
#include <Wire.h>
#include "buttons.h"
#include "eeprom.h"
#include "pinInits.h"
#include "packetSystem.h"

unsigned long buttonStates = 0UL;
unsigned long lastButtonStates = 0UL;
char lastPressedButton = -1;

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

unsigned long buttonsToEdit = 0UL;  // When recording state is entered, all buttons previous recordings to will replaced with new recording. 
                                    // This variable is exists to see if the recording should replace previous made recording or if the packet should be edited. 

void setup() {
  setupPinInit();

  #ifdef DEBUG_PRINTING
    serialPinInit();
    Serial.println("In debug mode. Sending will not work. ");
  #endif
  
  // Starting tasks
  switch (programState) {
    case remote:      
      #ifdef DEBUG_PRINTING
        Serial.println("Starting as remote...");
      #endif
      break;

    case recording:
      receivePinInit();
      lastPressedButton = -1;
      
      #ifdef DEBUG_PRINTING
        Serial.println("Starting in recording...");
      #endif
      break;

    default:
      break;
  }
}


void loop() {
  buttonsPinInit();
  scanMatrix(&buttonStates, &lastButtonStates, &lastPressedButton);
  #ifdef DEBUG_PRINTING
    serialPinInit();
  #endif
  
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
        IrReceiver.end();
        lastPressedButton = -1;
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
        lastPressedButton = -1;
        buttonsToEdit = 0UL;
        
        #ifdef DEBUG_PRINTING
          Serial.println("Switching to recording...");
          Serial.print("Available ram: ");
          Serial.println(freeRam(), DEC);
        #endif
        break;

      default:
        break;
    }
  }

  #ifdef DEBUG_PRINTING
    Serial.flush();
    Serial.end();
  #endif
  
  // Chose program routine
  switch (programState) {
    case remote:
      remoteProgram();
      break;

    case recording:
      receivePinInit();
      IrReceiver.begin(IR_RECEIVE_PIN);
      recordingProgram();
      break;

    default:
      remoteProgram();
      break;
  }
}


void remoteProgram() {
  for (unsigned char i = 0; i < NUMBER_OF_BUTTONS; i++) {
    // If a button is pressed
    if (buttonStates & buttonBitMask(i)) {
      I2CPinInit();
       
      unsigned char recordingsOnButton = readRecordingsOnButton(i);
      
      #ifdef DEBUG_PRINTING
        Wire.end();
        serialPinInit();
        Serial.print("Button pressed ");
        unsigned char tempRow, tempColumn;
        buttonDecimalToMatrice(&tempRow, &tempColumn, i);
        Serial.print(tempRow, DEC);
        Serial.print(tempColumn, DEC);
        Serial.print(": ");
        Serial.println(i, DEC);
        Serial.print("Bit mask: ");
        Serial.println(buttonStates, BIN);
        Serial.print("Recordings: ");
        Serial.println(recordingsOnButton, DEC);
        Serial.flush();
        Serial.end();
        I2CPinInit();
      #endif

      if (recordingsOnButton) {
        Recording buttonRecordings[recordingsOnButton];
        readButtonRecordings(buttonRecordings, i); // Read button recordings in packet
        Wire.end();
        
          // Loop through every recording and send it
          for (unsigned char k = 0; k < recordingsOnButton; k++) {
            if (lastButtonStates == buttonStates) {
                buttonRecordings[k].recordedIRData.flags = IRDATA_FLAGS_IS_REPEAT;
            }
            
            if (buttonRecordings[k].decodedFlag == RAW_FLAG) {

              #ifndef DEBUG_PRINTING // Not enough memory for both serial and IRSender
                sendPinInit();
                IrSender.sendRaw(buttonRecordings[k].rawCode, buttonRecordings[k].rawCodeLength, 38); // Send raw code. Assume 38 kHz carrier frequency
                
              #else 
                serialPinInit();
                Serial.print("Sending raw recording. ");
                Serial.print(buttonRecordings[k].rawCodeLength, DEC);
                Serial.println(" marks or spaces.");
                Serial.flush();
                Serial.end();
              #endif
              
            } else {

              #ifndef DEBUG_PRINTING // Not enough memory for both serial and IRSender
                sendPinInit();
                IrSender.write(&buttonRecordings[k].recordedIRData, NUMBER_OF_REPEATS);
                
              #else
                serialPinInit();
                Serial.print("Sending decoded recording. Protocol: ");
                Serial.print(buttonRecordings[k].recordedIRData.protocol, DEC); 
                Serial.print(" Command: ");
                Serial.println(buttonRecordings[k].recordedIRData.command, DEC);
                Serial.flush();
                Serial.end();
              #endif
            }
            
            if (recordingsOnButton > 1 && k < recordingsOnButton - 1) {
              delay(WAIT_BETWEEN_RECORDINGS); // Wait before sending next one
            }
          }
          
          delay(DELAY_BETWEEN_REPEAT); // Wait a bit between retransmissions
      } else {
        Wire.end();
      }
    }
  }

  // Only sleep if no buttons are pressed
  if (buttonStates == 0UL) {
    #ifdef DEBUG_PRINTING
      serialPinInit();
      Serial.println("Sleep initiating...\n");
      Serial.flush();
      Serial.end();
    #endif
    
    sleep();
    wakeProcedure();
    
    #ifdef DEBUG_PRINTING
      serialPinInit();
      Serial.println("Waking up...");
      Serial.flush();
      Serial.end();
    #endif
  }
}


// TODO: Make it possible to store multiple recordings on one button 
void recordingProgram() {
  #ifdef DEBUG_PRINTING
    for (unsigned char i = 0; i < sizeof(row); i++) {
      for (unsigned char j = 0; j < sizeof(column); j++) {
        if ((buttonStates & buttonBitMask(i, j)) && !(lastButtonStates & buttonBitMask(i, j))) {
          serialPinInit();
          Serial.print("\nButton pressed ");
          Serial.print(i, DEC);
          Serial.println(j, DEC);
          Serial.flush();
          Serial.end();
        }
      }
    }
  #endif 

  receivePinInit();
  // If there is recieved data and a button was pressed, decode. Only record when all buttons have been released for WAIT_AFTER_BUTTON since buttons on column 3 and 4 activate the sendere and that garbage can get recieved.
  if (IrReceiver.available() && (lastPressedButton != -1) && buttonStates == 0 && millis() - lastMillisButtonRecordingState > WAIT_AFTER_BUTTON) {   
    I2CPinInit();

    unsigned char numberOfRecordings = 1;
    if (buttonsToEdit & buttonBitMask(lastPressedButton)) {
      numberOfRecordings = readRecordingsOnButton(lastPressedButton) + 1;  

      #ifdef DEBUG_PRINTING
        Wire.end();
        serialPinInit();
        Serial.print("Adding to button packet. Packet will consist of ");
        Serial.print(numberOfRecordings, DEC);
        Serial.println(" recordings.");
        Serial.flush();
        Serial.end();
        I2CPinInit();
      #endif
    } else {
      buttonsToEdit |= buttonBitMask(lastPressedButton);
      
      #ifdef DEBUG_PRINTING
        Wire.end();
        serialPinInit();
        Serial.println("Creating new button packet. Discarding old recordings on button. ");
        Serial.flush();
        Serial.end();
        I2CPinInit();
      #endif
    }

    Recording buttonRecordings[numberOfRecordings]; // Make a buffer for the old recordings and the new one
    
    if (numberOfRecordings > 1) {
      readButtonRecordings(buttonRecordings, lastPressedButton); // Read the existing button recordings
      // Mark old packet as non-existing
      writeEEPROM(buttonInfoAddress(lastPressedButton), 0);
      writeEEPROM(buttonInfoAddress(lastPressedButton) + 1, 0);
    }

    IRData tempIRData = *IrReceiver.read(); // Read data

    #ifdef DEBUG_PRINTING
      Wire.end();
      serialPinInit();    
      Serial.print("IRResults: ");
      IrReceiver.printIRResultMinimal(&Serial);
      Serial.println();
      Serial.flush();
      Serial.end();
      I2CPinInit();
    #endif
    
    // If the protocol is unkown, the data needs to be saved raw
    if(tempIRData.protocol == UNKNOWN) {
      buttonRecordings[numberOfRecordings - 1].decodedFlag = RAW_FLAG; // Indicate that the recording is raw
      buttonRecordings[numberOfRecordings - 1].rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1; // Save the length of the recording. -1 since first byte is space since last recording I think...
      IrReceiver.compensateAndStoreIRResultInArray(buttonRecordings[numberOfRecordings - 1].rawCode); // Clean up the recording and save it
      
    } else {
      // If the data can be decoded, save it as IRData
      buttonRecordings[numberOfRecordings - 1].decodedFlag = DECODED_FLAG; // Indicate that the recording is raw
      buttonRecordings[numberOfRecordings - 1].recordedIRData = {tempIRData};
      //buttonRecordings[numberOfRecordings - 1].flags = 0; // clear flags -esp. repeat- for later sending
    }
    
    // Write the updated packet
    writeButtonPacket(buttonRecordings, numberOfRecordings, lastPressedButton);
    Wire.end();

    #ifdef DEBUG_PRINTING
      serialPinInit();    
      Serial.print("Recording saved on button  ");
      unsigned char tempRow;
      unsigned char tempColumn;
      buttonDecimalToMatrice(&tempRow, &tempColumn, lastPressedButton);
      Serial.print(tempRow, DEC);
      Serial.println(tempColumn, DEC);
      Serial.println();
      Serial.flush();
      Serial.end();
    #endif
    
    lastPressedButton = -1;
    
    receivePinInit();
    IrReceiver.start(IR_RECEIVE_PIN);
  }
      
  while (IrReceiver.decode()) {    
    #ifdef DEBUG_PRINTING
      serialPinInit();
      Serial.print("Flushing: ");
      IrReceiver.printIRResultMinimal(&Serial);
      Serial.println();
      Serial.flush();
      Serial.end();
    #endif

    IrReceiver.resume();
  } 
}


// Procedure to prepare for sleep and start sleeping
void sleep() {
  pinModeFast(IR_SEND_PIN, INPUT_PULLUP); // IR_SEND_PIN is normally high. Since it's the same pin as column 2, it will short column 0, 2 and 3 to permanently high.
                                      // That will not work to generate a falling interrupt. Therefore it needs to be an input. 

  pinModeFast(IR_RECEIVE_PIN, INPUT_PULLUP); // Leaving it floating will draw current

  // Set all rows to low so a falling interrupt can be generated when button is pressed 
  for (unsigned char i = 0; i < sizeof(row); i++) {
    pinMode(row[i], OUTPUT);
    digitalWrite(row[i], LOW);
  }
  
  // Short column 0, 2 and 3 so that they all connect to PIN_PA6 and all buttons can generate interrupts. 
  pinModeFast(SHORT_COLUMNS, OUTPUT);
  digitalWriteFast(SHORT_COLUMNS, LOW); // Active low

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

  // Revert columns to high
  for (unsigned char i = 0; i < sizeof(column); i++) {
    pinMode(column[i], OUTPUT);
    digitalWrite(column[i], HIGH);
  }
}

int freeRam() {

  extern int __heap_start,*__brkval;

  int v;

  return (int)&v - (__brkval == 0  

    ? (int)&__heap_start : (int) __brkval);  

}
