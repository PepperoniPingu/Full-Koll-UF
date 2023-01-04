#include "packetSystem.h"

// Each button has a permanent address for info. The info contains another address to where a "button packet" is found. A button packet contains 
// number of signals, a flag if it is raw or decoded and the corresponding data. If the flag indicates it is raw then the next byte indicates how long the recording is in bytes. If it is decoded then the next bytes are a direct copy of 
// IRData. The info is 2 bytes which are the address of the button packet. 
// This function returns the address for the permanent address for the button info. 
unsigned int buttonInfoAddress(unsigned char buttonDecimal) {
  unsigned int tempInfoAddress = 2 * buttonDecimal;
  return tempInfoAddress;
}


// Function to return the dynamic address of a button packet. 
unsigned int buttonPacketAddress(unsigned char buttonDecimal) {

  // Read the adress of the button packet
  unsigned int tempButtonPacketAddress = 0;
  tempButtonPacketAddress = readEEPROM(buttonInfoAddress(buttonDecimal)) << 8;
  tempButtonPacketAddress |= readEEPROM(buttonInfoAddress(buttonDecimal) + 1);

  // Reset the info packet and return 0 if one of following conditions is true: 
  if ((tempButtonPacketAddress > MAX_EEPROM_ADDRESS - sizeof(IRData) - 1) ||                                    // The addres to the buttonPacket is bigger than available memory
        (tempButtonPacketAddress < buttonInfoAddress(NUMBER_OF_BUTTONS) && tempButtonPacketAddress != 0)) {   // The address is in the button info region (first 40 bytes)
    writeEEPROM(buttonInfoAddress(buttonDecimal), 0);
    writeEEPROM(buttonInfoAddress(buttonDecimal) + 1, 0);
    tempButtonPacketAddress = 0;
  }

  return tempButtonPacketAddress;
}


// First byte of the button packet contains the number of recordings
// Second byte contains decoded flag
// Third byte and after contains the first recording's IRData or the raw data
void readButtonRecording(Recording* recording,unsigned char recordingIndex, unsigned char buttonDecimal) {

  unsigned int tempButtonPacketAddress = buttonPacketAddress(buttonDecimal); // Find the address of the button packet

  // If the packet address is 0 then there is no packet and return 0
  if (tempButtonPacketAddress != 0) {
    
    unsigned char tempNumberOfRecordings = readRecordingsOnButton(buttonDecimal); // Read the number of recordings

    // Only return button packets if there are any
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF && recordingIndex < tempNumberOfRecordings) {

      unsigned int lengthsSum = 0;
      
      // Loop through every recording and read their sizes to get the address of the recording indicated in argument recordingIndex
      for (unsigned char i = 0; i < recordingIndex; i++) { 
        
        if (readEEPROM(tempButtonPacketAddress + 1 + lengthsSum) == RAW_FLAG) { // Read if it is raw format or IRData
          lengthsSum += readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 1) + 2;  
        } else {          
          lengthsSum += sizeof(IRData) + 1;
        }
      }

      recording->decodedFlag = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum); // Read if it is raw format or IRData

      #ifdef DEBUG_EEPROM_PRINTING
        Wire.end();
        serialPinInit();
        Serial.print("Attempting to read recording with index ");
        Serial.println(recordingIndex, DEC);
        Serial.print("R A");
        Serial.print(tempButtonPacketAddress + 1 + lengthsSum, DEC);
        Serial.print(" D");
        Serial.print(recording->decodedFlag, DEC);
        Serial.println(" (decodedFlag)");
        serialPinDeInit();
        I2CPinInit();
      #endif

      // Read raw recording
      if (recording->decodedFlag == RAW_FLAG) {
          
          recording->rawCodeLength = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 1); // Read length of raw data

          // Loop through every byte in said recording and save it
          for (unsigned char i = 0; i < recording->rawCodeLength; i++) {
            recording->rawCode[i] = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 2 + i); // Read byte
            
            #ifdef DEBUG_EEPROM_PRINTING
              Wire.end();
              serialPinInit();
              Serial.print("R A");
              Serial.print(tempButtonPacketAddress + 1 + lengthsSum + 2 + i, DEC);
              Serial.print(" D");
              Serial.println(recording->rawCode[i], DEC);
              serialPinDeInit();
              I2CPinInit();
            #endif
          }

        // Read decoded recording
        } else {

          for (unsigned char i = 0; i < sizeof(IRData); i++) {
            *((unsigned char*) &recording->recordedIRData + i) = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 1 + i); // Read byte
            
            #ifdef DEBUG_EEPROM_PRINTING
              Wire.end();
              serialPinInit();
              Serial.print("R A");
              Serial.print(tempButtonPacketAddress + 1 + lengthsSum + 1 + i, DEC);
              Serial.print(" D");
              Serial.println(*((unsigned char*) &recording->recordedIRData + i), DEC);//IRDataBuffer[i], DEC);
              serialPinDeInit();
              I2CPinInit();
            #endif
          }
        }
      
    } else {
      // If there are 0 recordings here, reset the info data
      unsigned int tempButtonInfoAddress = buttonInfoAddress(buttonDecimal);
      writeEEPROM(tempButtonInfoAddress, 0);
      writeEEPROM(tempButtonInfoAddress + 1, 0);
    }
  }
}


// Returns number of recordings in a packet
unsigned char readRecordingsOnButton(unsigned char buttonDecimal) {
  unsigned int tempButtonPacketAddress = buttonPacketAddress(buttonDecimal); // Find the address of the button packet

  // If the packet address is 0 then there is no packet and return 0
  if (tempButtonPacketAddress != 0) {
    unsigned char tempNumberOfRecordings = readEEPROM(tempButtonPacketAddress); // Read the number of recordings
    
    // Take action if there are 0 recordings 
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF) {
        return tempNumberOfRecordings; // Return packet length
        
    } else {
      // If there are 0 recordings here, reset the info data
      unsigned int tempButtonInfoAddress = buttonInfoAddress(buttonDecimal);
      writeEEPROM(tempButtonInfoAddress, 0);
      writeEEPROM(tempButtonInfoAddress + 1, 0);
    }
  }
  
  return 0;
}


// Returns total length of button packet
unsigned int readButtonPacketLength(unsigned char buttonDecimal) {
  unsigned char tempRecordings = readRecordingsOnButton(buttonDecimal);
  if (tempRecordings != 0) {
    unsigned int tempButtonPacketAddress = buttonPacketAddress(buttonDecimal);

    unsigned int sumRecordingLengths = 1; // First byte is number of recordings 
    
    for (unsigned char i = 0; i < tempRecordings; i++) {
      if (readEEPROM(tempButtonPacketAddress + sumRecordingLengths) == RAW_FLAG) { // If recording is raw or decoded
        sumRecordingLengths += readEEPROM(tempButtonPacketAddress + sumRecordingLengths + 1) + 2; // One extra byte consisting of decodedFlag
      } else {
        sumRecordingLengths += sizeof(IRData) + 1;
      }
    }
    
    return sumRecordingLengths;
  }
  return 0;
}


void writeButtonRecording(unsigned int recordingAddress, Recording* recording) {

  writeEEPROM(recordingAddress, recording->decodedFlag & 0x1); // Write if the recording is raw or decoded in to the first byte of the recording

  // Write raw recording
  if (recording->decodedFlag == RAW_FLAG) {

    #ifdef DEBUG_PRINTING
      Wire.end();
      serialPinInit();
      Serial.println("Saving raw recording...");
      Serial.print("Writing ");
      Serial.print(recording->rawCodeLength, DEC);
      Serial.println(" bytes.");
      serialPinDeInit();
      I2CPinInit();
    #endif
    
    writeEEPROM(recordingAddress + 1/*recording type*/, recording->rawCodeLength); // Write how long the recording is in the second byte of the recording

    // Write the raw data
    for (unsigned int i = 0; i < recording->rawCodeLength; i++) {
      writeEEPROM(recordingAddress + 2/*recording type and recording length*/ + i, recording->rawCode[i]);

      #ifdef DEBUG_EEPROM_PRINTING
        Wire.end();
        serialPinInit();
        Serial.print("W A");
        Serial.print(recordingAddress + 2/*recording type and recording length*/ + i, DEC);
        Serial.print(" D");
        Serial.println(recording->rawCode[i], DEC);
        serialPinDeInit();
        I2CPinInit();
      #endif
    }

  // Decoded recording
  } else {

    #ifdef DEBUG_PRINTING
        Wire.end();
        serialPinInit();
        Serial.println("Saving decoded recording...");
        Serial.print("Writing ");
        Serial.print(sizeof(IRData), DEC);
        Serial.println(" bytes.\n");
        serialPinDeInit();
        I2CPinInit();
      #endif

    // Write the raw data
    for (unsigned int i = 0; i < sizeof(IRData); i++) {
      writeEEPROM(recordingAddress + 1/*recording type*/ + i, *((unsigned char*) &recording->recordedIRData + i));

      #ifdef DEBUG_EEPROM_PRINTING
        Wire.end();
        serialPinInit();
        Serial.print("W A");
        Serial.print(recordingAddress + 1/*recording type*/ + i, DEC);
        Serial.print(" D");
        Serial.println(*((unsigned char*) &recording->recordedIRData + i), DEC);
        serialPinDeInit();
        I2CPinInit();
      #endif
    }
  }

}

// Creates a new button packet
// TODO: add error handling if given tempAddress is not allowed, for example 0
void createButtonPacket(Recording* recording, unsigned char buttonDecimal) {

  // Determine size of packet
  unsigned int buttonPacketSize;
  if (recording->decodedFlag == RAW_FLAG) {
    buttonPacketSize = recording->rawCodeLength + 2; // +2 since there is one byte for type of recording and one byte for length of recording
  } else {
    buttonPacketSize = sizeof(IRData) + 1; // +1 since there is one byte for type of recording. 
  }

  unsigned int tempAddress = scanEmptyEEPROMAddresses(buttonPacketSize, CREATING_PACKET); // Find an empty address space where the packet can be put
  
  #ifdef DEBUG_PRINTING
    Wire.end();
    serialPinInit();
    Serial.print("\nWriting recording to address ");
    Serial.println(tempAddress, DEC);
    serialPinDeInit();
    I2CPinInit();
  #endif
  
  // Update button info. Change the address to the new found one.
  writeEEPROM(buttonInfoAddress(buttonDecimal), tempAddress >> 8);  
  writeEEPROM(buttonInfoAddress(buttonDecimal) + 1, tempAddress); 

  writeEEPROM(tempAddress, 1); // First byte is number of recordings saved

  writeButtonRecording(tempAddress + 1, recording); // Write recording
}


// Update button packet by adding a recording to the end of the packet
void pushButtonRecording(Recording* recording, unsigned char buttonDecimal) {

  // Determine if there is enough space after the existing packet to add a recording
  bool canPutRecordingAfter = false;
  if (recording->decodedFlag == RAW_FLAG) {
    canPutRecordingAfter = scanEmptyEEPROMAddresses(recording->rawCodeLength + 2, buttonDecimal);
    
  } else {
    canPutRecordingAfter = scanEmptyEEPROMAddresses(sizeof(IRData) + 1, buttonDecimal);
  }

  unsigned int tempButtonPacketAddress = buttonPacketAddress(buttonDecimal);
  unsigned char numberOfRecordings = readRecordingsOnButton(buttonDecimal) + 1;

  // Loop through every recording and read their sizes to get the total length of all the recordings. Obs, doesn't include the extra byte for the number of recordings
  unsigned int lengthsSum = 0;
  for (unsigned char i = 0; i < numberOfRecordings - 1; i++) { 
    
    if (readEEPROM(tempButtonPacketAddress + 1/*number of recordings*/ + lengthsSum) == RAW_FLAG) { // Read if it is raw format or IRData
      lengthsSum += readEEPROM(tempButtonPacketAddress + 1/*number of recordings*/  + lengthsSum + 1/*type of recording*/) + 2;  
    } else {          
      lengthsSum += sizeof(IRData) + 1/*type of recording*/;
    }
  }

  // If there is enoguh space behind the packet, put the new recording there
  if (canPutRecordingAfter) {

    writeEEPROM(tempButtonPacketAddress, numberOfRecordings); // Update the number of recordings

    writeButtonRecording(tempButtonPacketAddress + 1/*number of recordings*/ + lengthsSum, recording); // Write the recording

  // If there is not enough space after the packet, find a new empty space and copy the packet there
  } else {

    // Find new empty space
    unsigned int newButtonPacketAddress = scanEmptyEEPROMAddresses(lengthsSum + (recording->decodedFlag == RAW_FLAG? recording->rawCodeLength + 2: sizeof(IRData) + 1), CREATING_PACKET);

    // Copy each byte except the number of recordings
    for (unsigned int i = 0; i < lengthsSum; i++) {
      writeEEPROM(newButtonPacketAddress + 1/*don't copy number of recordings*/ + i, readEEPROM(tempButtonPacketAddress + 1 /*don't copy number of recordings*/));
    }

    // Update button info. Change the address to the new found one.
    writeEEPROM(buttonInfoAddress(buttonDecimal), newButtonPacketAddress >> 8);  
    writeEEPROM(buttonInfoAddress(buttonDecimal) + 1, newButtonPacketAddress);

    writeEEPROM(newButtonPacketAddress, numberOfRecordings); // Update the number of recordings

    writeButtonRecording(newButtonPacketAddress + 1/*number of recordings*/ + lengthsSum, recording); // Write the recording
  }
}


// Find an address where the bytes required can fit. Will return the biggest free address space
// Set emptyAfterButtonDecimal to CREATE_PACKET to look for the largest free address space. 
// Set it to a button code to answer the question "Is there enough empty space behind this packet to extend the packet with another recording?"
// TODO: Make it so that spaces can't be nagative and so that they can't overlap
unsigned int scanEmptyEEPROMAddresses(unsigned int bytesRequired, char emptyAfterButtonDecimal) {
  
  unsigned int packetAddresses[NUMBER_OF_BUTTONS + 2];
  unsigned int packetLengths[sizeof(packetAddresses) / sizeof(unsigned int)];

  // Retrieve addresses and lengths of all packets
  for (unsigned char i = 0; i < NUMBER_OF_BUTTONS; i++) {
    packetAddresses[i] = buttonPacketAddress(i);  // Retreive address of packet
    packetLengths[i] = readButtonPacketLength(i); // Retrieve length of packet
  }
  // Make an entry for the sapce occupied by the button info data
  packetAddresses[NUMBER_OF_BUTTONS] = 0;
  packetLengths[NUMBER_OF_BUTTONS] = buttonInfoAddress(NUMBER_OF_BUTTONS - 1) + 2;
  // Make an entry the end of the chip
  packetAddresses[NUMBER_OF_BUTTONS + 1] = MAX_EEPROM_ADDRESS;
  packetLengths[NUMBER_OF_BUTTONS + 1] = 0;

  // Sort addresses and lengths, small to large
  for (unsigned char i = 0; i < NUMBER_OF_BUTTONS; i++) {
    for (unsigned char j = i + 1; j < NUMBER_OF_BUTTONS + 1; j++) {
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
    Wire.end();
    serialPinInit();
    // Prints empty address spaces
    Serial.print("Searching for empty address space that fits ");
    Serial.print(bytesRequired, DEC);
    Serial.println(" bytes...");
    Serial.println("Available spaces: ");
    for (unsigned char i = 0; i < NUMBER_OF_BUTTONS + 1; i++) {
      Serial.print(i);
      Serial.print(" : ");
      Serial.print(packetAddresses[i] + packetLengths[i], DEC);
      Serial.print(" -> ");
      Serial.println(packetAddresses[i + 1], DEC);
    }
    Serial.println();
    serialPinDeInit();
    I2CPinInit();
  #endif

  // Retrieve button packet address if looking at a specific button 
  unsigned int tempAddress = 0;
  if (emptyAfterButtonDecimal != CREATING_PACKET) {
    tempAddress = buttonPacketAddress(emptyAfterButtonDecimal);
  }
  // Find the biggest free address space and return that address or see if there is enough space behind a specific button packet
  unsigned int longestSpaceAddress = 0;
  unsigned int addressSpace = 0;
  unsigned int bestAddress = 0;
  // Iterate through every button packet
  for (unsigned char i = 0; i < NUMBER_OF_BUTTONS + 1; i++) {
    addressSpace = packetAddresses[i+1] - (packetAddresses[i] + packetLengths[i]); // Save the address of the empty space

    // Determine if looking for longest address space or space after button
    if (emptyAfterButtonDecimal == CREATING_PACKET) {
      
      // Compare the empty space, if it is large enough but also the largest one discover yet, save it. 
      if (addressSpace >= bytesRequired && addressSpace > longestSpaceAddress) {
        longestSpaceAddress = addressSpace;
        bestAddress = packetAddresses[i] + packetLengths[i];
      }
      
    // Looking if there is enough space after button packet
    } else if (packetAddresses[i] == tempAddress && addressSpace >= bytesRequired) {
      return true;
    }
  }
  
  return bestAddress;
}
