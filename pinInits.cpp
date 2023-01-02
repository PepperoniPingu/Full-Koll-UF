#include "pinInits.h"

void setupPinInit() {
  //Initialize all the rows as outputs
  pinModeFast(PIN_PB0, OUTPUT);
  digitalWriteFast(PIN_PB0, HIGH);
  pinModeFast(PIN_PB1, OUTPUT);
  digitalWriteFast(PIN_PB1, HIGH);
  pinModeFast(PIN_PA1, OUTPUT);
  digitalWriteFast(PIN_PA1, HIGH);
  pinModeFast(PIN_PA5, OUTPUT);
  digitalWriteFast(PIN_PA5, HIGH);
  pinModeFast(PIN_PA3, OUTPUT);
  digitalWriteFast(PIN_PA3, HIGH);

  // Make the columns OUTPUT and HIGH, otherwise, Serial will interfer with them and activate the IR LED
  pinModeFast(PIN_PB3, OUTPUT);
  digitalWriteFast(PIN_PB3, HIGH);
  pinModeFast(PIN_PA2, OUTPUT);
  digitalWriteFast(PIN_PA2, HIGH);
  pinModeFast(PIN_PA4, OUTPUT);
  digitalWriteFast(PIN_PA4, HIGH);
  pinModeFast(PIN_PA6, OUTPUT);
  digitalWriteFast(PIN_PA6, HIGH);
}

void I2CPinInit() {
  // Initialize all the columns as inputs. Necessary if a button is pressed and connects one of the I2C pins (that is also a row) to a column
  pinModeFast(PIN_PB3, INPUT_PULLUP);
  pinModeFast(PIN_PA2, INPUT_PULLUP);
  pinModeFast(PIN_PA4, INPUT_PULLUP);
  pinModeFast(PIN_PA6, INPUT_PULLUP);

  //Initialize all the rows as INPUTS
  pinModeFast(PIN_PB0, INPUT_PULLUP);
  pinModeFast(PIN_PB1, INPUT_PULLUP);
  pinModeFast(PIN_PA1, INPUT_PULLUP);
  pinModeFast(PIN_PA5, INPUT_PULLUP);
  pinModeFast(PIN_PA3, INPUT_PULLUP);

   
  // Activate SHORT_COLUMNS so that we can use the pull up from the IR LED. 
  pinModeFast(SHORT_COLUMNS, OUTPUT); 
  digitalWriteFast(SHORT_COLUMNS, LOW); // Active low
  
  Wire.swap(0);
  Wire.usePullups();
  Wire.begin();
}

void serialPinInit() { 
  // Make the columns OUTPUT and HIGH, otherwise, Serial will interfer with them and activate the IR LED since SHORT_COLUMNS is serial TX. 
  pinModeFast(PIN_PB3, OUTPUT);
  digitalWriteFast(PIN_PB3, HIGH);
  pinModeFast(PIN_PA4, OUTPUT);
  digitalWriteFast(PIN_PA4, HIGH);
  pinModeFast(PIN_PA6, OUTPUT);
  digitalWriteFast(PIN_PA6, HIGH);
  
  Serial.swap(0); // Use serial interface 0
  Serial.begin(SERIAL_SPEED, SERIAL_TX_ONLY);
}

void buttonsPinInit() {  
  // SHORT_COLUMS is active low and needs to be disabled to read individual button presses. 
  pinModeFast(SHORT_COLUMNS, OUTPUT); 
  digitalWriteFast(SHORT_COLUMNS, HIGH); // Active low

  // Initialize all the columns as inputs
  pinModeFast(PIN_PB3, INPUT_PULLUP);
  pinModeFast(PIN_PA2, INPUT_PULLUP);
  pinModeFast(PIN_PA4, INPUT_PULLUP);
  pinModeFast(PIN_PA6, INPUT_PULLUP);

  // PIN_PA4 is used for both button matrix and sending IR. When sending IR-codes it needs to be inverted since the IR LED is active low. Make sure it is not inverted now. 
  PORTA.PIN4CTRL &= ~PORT_INVEN_bm;

  //Initialize all the rows as outputs
  pinModeFast(PIN_PB0, OUTPUT);
  digitalWriteFast(PIN_PB0, HIGH);
  pinModeFast(PIN_PB1, OUTPUT);
  digitalWriteFast(PIN_PB1, HIGH);
  pinModeFast(PIN_PA1, OUTPUT);
  digitalWriteFast(PIN_PA1, HIGH);
  pinModeFast(PIN_PA5, OUTPUT);
  digitalWriteFast(PIN_PA5, HIGH);
  pinModeFast(PIN_PA3, OUTPUT);
  digitalWriteFast(PIN_PA3, HIGH);
}

void receivePinInit(){
  #ifdef DEBUG_PRINTING
    // Make the columns OUTPUT and HIGH, otherwise, Serial will interfer with them and activate the IR LED since SHORT_COLUMNS is serial TX. 
    pinModeFast(PIN_PB3, OUTPUT);
    digitalWriteFast(PIN_PB3, HIGH);
    pinModeFast(PIN_PA4, OUTPUT);
    digitalWriteFast(PIN_PA4, HIGH);
    pinModeFast(PIN_PA6, OUTPUT);
    digitalWriteFast(PIN_PA6, HIGH);
  #else
    // SHORT_COLUMS can disconnect the IR LED if it is not used for serial 
    pinModeFast(SHORT_COLUMNS, OUTPUT); 
    digitalWriteFast(SHORT_COLUMNS, HIGH); // Active low
  #endif
    
}

void sendPinInit(){ 
  // Disable the shorting of columns since this will also short the IR LED
  pinModeFast(SHORT_COLUMNS, OUTPUT);
  digitalWriteFast(SHORT_COLUMNS, HIGH);
  
  IrSender.enableIROut(38);
  IrSender.begin(IR_SEND_PIN);
  PORTA.PIN4CTRL |= PORT_INVEN_bm; // The LED is controlled by a MOSFET that is active low. Therefore the signal needs to be inverted. 
  digitalWriteFast(IR_SEND_PIN, LOW);
}
