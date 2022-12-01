#define EXCLUDE_EXOTIC_PROTOCOLS enabled
#define NO_LED_FEEDBACK_CODE enabled
#define RAW_BUFFER_LENGTH 100

#define IR_RECEIVE_PIN PIN_PA7
#define IR_SEND_PIN PIN_PA4

#define SHORT_COLUMNS PIN_PB2

#define NUMBER_OF_REPEATS 3

#include <IRremote.hpp>
#include "buttons.cpp"

const unsigned char row[5] = {PIN_PB0, PIN_PB1, PIN_PA1, PIN_PA5, PIN_PA3};
const unsigned char column[4] = {PIN_PB3, PIN_PA2, PIN_PA4, PIN_PA6};

unsigned long buttonStates = 0UL;
unsigned long lastButtonStates = 0UL;
char lastPressedButton[2] = { -1, -1};

enum ProgramState : unsigned char {
  remote,
  recording,
  stateCount
};

ProgramState programState = remote;

// Storage for the recorded code
struct storedIRDataStruct {
  IRData receivedIRData;
} storedIRData[5][4];

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
}

void loop() {
  
  // Serial doesn't work when SHORT_COLUMNS is high. Therefore, to scan the button matrix, serial has to be disabled. 
  Serial.end();
  // SHORT_COLUMS is active low and needs to be disabled to read individual button presses. 
  pinMode(SHORT_COLUMNS, OUTPUT); 
  digitalWrite(SHORT_COLUMNS, HIGH);
  scanMatrix();
  Serial.begin(9600); // Turn serial back on once the button scanning is done


  // Switch program state when btn41 is pressed
  if ((buttonStates & buttonBitMasks[4][1]) && !(lastButtonStates & buttonBitMasks[4][1])) {
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

      if ((buttonStates & buttonBitMasks[i][j]) && !(lastButtonStates & buttonBitMasks[i][j])) {
        Serial.print("Knapp tryckt ");
        Serial.print(i, DEC);
        Serial.println(j, DEC);
        Serial.println(buttonStates, BIN);
        Serial.print("Protocol: ");
        Serial.println(storedIRData[i][j].receivedIRData.protocol);
        Serial.print("Command: ");
        Serial.println(storedIRData[i][j].receivedIRData.command);
        IrSender.write(&storedIRData[i][j].receivedIRData, NUMBER_OF_REPEATS);
      }
    }
  }

}

void recordingProgram() {
  for (unsigned char i = 0; i < sizeof(row); i++) {
    for (unsigned char j = 0; j < sizeof(column); j++) {
      if (buttonStates & buttonBitMasks[i][j]) {
        lastPressedButton[0] = i;
        lastPressedButton[1] = j;
        Serial.print(i, DEC);
        Serial.println(j, DEC);
      }
    }
  }

  // If there is recieved data and a button was pressed, decode
  if (IrReceiver.decode() && (lastPressedButton[0] != -1) && (lastPressedButton[1] != -1)) {
      IrReceiver.printIRResultShort(&Serial);
      storedIRData[lastPressedButton[0]][lastPressedButton[1]].receivedIRData = *IrReceiver.read();
      Serial.print("kod sparad på knapp ");
      Serial.print(lastPressedButton[0], DEC);
      Serial.println(lastPressedButton[1], DEC);

      lastPressedButton[0] = -1;
      lastPressedButton[1] = -1;

      IrReceiver.resume();

      while (IrReceiver.available()) {
        IrReceiver.printIRResultShort(&Serial);
        IrReceiver.resume();
      }
    }


  while (IrReceiver.available()) {
    IrReceiver.read();
    IrReceiver.resume();
  }
}


// Function for reading the buttons
void scanMatrix() {
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
  
        if (buttonStates & buttonBitMasks[i][j]) {
          lastPressedButton[0] = i;
          lastPressedButton[1] = j;
        }
      }
      digitalWrite(row[i], HIGH);
    }
  
    // Correct for hardware bug on last column (column 3)
    if (buttonStates & buttonBitMasks[0][3]) {
      buttonStates = buttonStates & ~(buttonBitMasks[0][2] | buttonBitMasks[0][0]);
    }
    if (buttonStates & buttonBitMasks[1][3]) {
      buttonStates = buttonStates & ~(buttonBitMasks[1][2] | buttonBitMasks[1][0]);
    }
    if (buttonStates & buttonBitMasks[2][3]) {
      buttonStates = buttonStates & ~(buttonBitMasks[2][2] | buttonBitMasks[2][0]);
    }
    if (buttonStates & buttonBitMasks[3][3]) {
      buttonStates = buttonStates & ~(buttonBitMasks[3][2] | buttonBitMasks[3][0]);
    }
    if (buttonStates & buttonBitMasks[4][3]) {
      buttonStates = buttonStates & ~(buttonBitMasks[4][2] | buttonBitMasks[4][0]);
    }

  } while (tempButtonStates != buttonStates); // Compare so that the two runs have got the same value

  // Revert IR_SEND_PIN so it can be used for sending IR
  PORTA.PIN4CTRL |= PORT_INVEN_bm;
  pinMode(IR_SEND_PIN, OUTPUT);
  digitalWrite(IR_SEND_PIN, HIGH);
}

/*void sleep() {
  TCB0.CTRLA = 0; // Disable TCB0 timer
  ADC0.CTRLA &= ~ADC_ENABLE_bm; // Disable ADC
  
}*/

/*void wakeProcedure() {
  init_TCA0(); // Wake TCA0 timer
  init_millis(); // Initialize millis
}*/
