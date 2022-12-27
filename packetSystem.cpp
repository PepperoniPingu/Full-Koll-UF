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
unsigned char readButtonRecordings(Recording recordings[], unsigned char buttonDecimal) {

  unsigned int tempButtonPacketAddress = buttonPacketAddress(buttonDecimal); // Find the address of the button packet

  // If the packet address is 0 then there is no packet and return 0
  if (tempButtonPacketAddress != 0) {
    
    unsigned char tempNumberOfRecordings = readRecordingsOnButton(buttonDecimal); // Read the number of recordings
    //Serial.print("NumberOfRecordings ");
    //Serial.println(tempNumberOfRecordings, DEC);
    // Only return button packets if there are any
    if (tempNumberOfRecordings != 0 && tempNumberOfRecordings != 0xFF) {

      unsigned int lengthsSum = 0;
      
      // Loop through every recording
      for (unsigned char i = 0; i < tempNumberOfRecordings; i++) { 
        
        recordings[i].decodedFlag = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum) & 0x1; // Read if it is raw format or IRData

        #ifdef DEBUG_PRINTING
          Wire.end();
          serialPinInit();
          Serial.print("Reading decodedFlag from address ");
          Serial.print(tempButtonPacketAddress + 1 + lengthsSum, DEC);
          Serial.print(". Read ");
          Serial.println(recordings[i].decodedFlag, DEC);
          Serial.flush();
          Serial.end();
          I2CPinInit();
        #endif

        if (recordings[i].decodedFlag == RAW_FLAG) {
          recordings[i].rawCodeLength = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 1); // Read length of raw data

          // Loop through every byte in said recording and save it
          for (unsigned char j = 0; j < recordings[i].rawCodeLength; j++) {
            recordings[i].rawCode[j] = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 2 + j); // Read byte
            
            #ifdef DEBUG_PRINTING
              Wire.end();
              serialPinInit();
              Serial.print("Recording: ");
              Serial.print(i, DEC);
              Serial.print(" Byte number: ");
              Serial.print(j, DEC);
              Serial.print(" Address: ");
              Serial.print(tempButtonPacketAddress + 1 + lengthsSum + 2 + j, DEC);
              Serial.print(" Byte: ");
              Serial.println(recordings[i].rawCode[j], DEC);
              Serial.flush();
              Serial.end();
              I2CPinInit();
            #endif
          }

          lengthsSum += recordings[i].rawCodeLength + 2;
        } else {

          unsigned char IRDataBuffer[sizeof(IRData)];
          
          for (unsigned char j = 0; j < sizeof(IRData); j++) {
            IRDataBuffer[j] = readEEPROM(tempButtonPacketAddress + 1 + lengthsSum + 1 + j); // Read byte
            
            #ifdef DEBUG_PRINTING
              Wire.end();
              serialPinInit();
              Serial.print("Recording: ");
              Serial.print(i, DEC);
              Serial.print(" Byte number: ");
              Serial.print(j, DEC);
              Serial.print(" Address: ");
              Serial.print(tempButtonPacketAddress + 1 + lengthsSum + 1 + j, DEC);
              Serial.print(" Byte: ");
              Serial.println(IRDataBuffer[j], DEC);
              Serial.flush();
              Serial.end();
              I2CPinInit();
            #endif
          }

          memcpy(&recordings[i].recordedIRData, IRDataBuffer, sizeof(IRData));   // Save the recording in IRData format
          
          lengthsSum += sizeof(IRData) + 1;
        }
        
      } 

      return tempNumberOfRecordings;  // Return number of recordings
      
    } else {
      // If there are 0 recordings here, reset the info data
      unsigned int tempButtonInfoAddress = buttonInfoAddress(buttonDecimal);
      writeEEPROM(tempButtonInfoAddress, 0);
      writeEEPROM(tempButtonInfoAddress + 1, 0);
    }
  }

  // If earlier return didn't complete, return o. 
  return 0;
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
      if (readEEPROM(tempButtonPacketAddress + 1 + sumRecordingLengths) == RAW_FLAG) { // If recording is raw or decoded
        sumRecordingLengths += readEEPROM(tempButtonPacketAddress + 1 + sumRecordingLengths + 1) + 2; // Two extra bytes consisting of decodedFlag and length of raw code
      } else {
        sumRecordingLengths += sizeof(IRData) + 1;
      }
    }
    
    return sumRecordingLengths;
  }
  return 0;
}


// Writes a button packet
// TODO: add error handling if given tempAddress is not allowed, for example 0
void writeButtonPacket(Recording recordings[], unsigned char numberOfRecordings, unsigned char buttonDecimal) {

  unsigned int buttonPacketSize = 0;
  for (unsigned char i = 0; i < numberOfRecordings; i++) {
    if (recordings[i].decodedFlag == RAW_FLAG) {
      buttonPacketSize += recordings[i].rawCodeLength + 2; // +2 since there is one byte for type of recording and one byte for length of recording
    } else {
      buttonPacketSize += sizeof(IRData) + 1; // +1 since there is one byte for type of recording. 
    }
  }

  unsigned int tempAddress = scanEmptyEEPROMAddresses(buttonPacketSize); // Find an empty address space where the packet can be put

  #ifdef DEBUG_PRINTING
    Wire.end();
    serialPinInit();
    Serial.print("Writing ");
    Serial.print(numberOfRecordings, DEC);
    Serial.print(" recordings to address ");
    Serial.println(tempAddress, DEC);
    Serial.flush();
    Serial.end();
    I2CPinInit();
  #endif
  
  // Update button info. Change the address to the new found one.
  writeEEPROM(buttonInfoAddress(buttonDecimal), tempAddress >> 8);  
  writeEEPROM(buttonInfoAddress(buttonDecimal) + 1, tempAddress); 

  writeEEPROM(tempAddress, numberOfRecordings); // First byte is number of recordings saved

  // Write every recording in order

  unsigned int sumRecordingLengths = 0;
  for (unsigned char i = 0; i < numberOfRecordings; i++) {
  
    writeEEPROM(tempAddress + 1 + sumRecordingLengths, recordings[i].decodedFlag & 0x1); // Write if the recording is raw or decoded in to the first byte of the recording
    
    #ifdef DEBUG_PRINTING
        Wire.end();
        serialPinInit();
        Serial.print("Writing decodedFlag to address ");
        Serial.print(tempAddress + 1 + sumRecordingLengths, DEC);
        Serial.print(". Wrote ");
        Serial.println(recordings[i].decodedFlag & 0x1, DEC);
        Serial.flush();
        Serial.end();
        I2CPinInit();
      #endif
    
    if (recordings[i].decodedFlag == RAW_FLAG) {

      #ifdef DEBUG_PRINTING
          Wire.end();
          serialPinInit();
          Serial.println("Saving raw recording...");
          Serial.print("Writing ");
          Serial.print(recordings[i].rawCodeLength, DEC);
          Serial.print(" bytes.");
          Serial.flush();
          Serial.end();
          I2CPinInit();
        #endif
      
      writeEEPROM(tempAddress + 1 + sumRecordingLengths + 1, recordings[i].rawCodeLength); // Write how long the recording is in the second byte of the recording

      // Copy data in to byte array
      unsigned char writeBuffer[recordings[i].rawCodeLength];
      memcpy(writeBuffer, recordings[i].rawCode, recordings[i].rawCodeLength);

      // Write the raw data
      for (unsigned int j = 0; j < recordings[i].rawCodeLength; j++) {
        writeEEPROM(tempAddress + 1 + sumRecordingLengths + 2 + j, writeBuffer[j]);

        #ifdef DEBUG_PRINTING
          Wire.end();
          serialPinInit();
          Serial.print("Writing ");
          Serial.print(writeBuffer[j], DEC);
          Serial.print(" to address ");
          Serial.println(tempAddress + 1 + sumRecordingLengths + 2 + j, DEC);
          Serial.flush();
          Serial.end();
          I2CPinInit();
        #endif
      }

      sumRecordingLengths += recordings[i].rawCodeLength + 2;

    // Decoded recording
    } else {

      #ifdef DEBUG_PRINTING
          Wire.end();
          serialPinInit();
          Serial.println("Saving decoded recording...");
          Serial.print("Writing ");
          Serial.print(sizeof(IRData), DEC);
          Serial.println(" bytes.\n");
          Serial.flush();
          Serial.end();
          I2CPinInit();
        #endif

      // Copy data in to byte array
      unsigned char writeBuffer[sizeof(IRData)];
      memcpy(writeBuffer, &recordings[i].recordedIRData, sizeof(IRData));

      // Write the raw data
      for (unsigned int j = 0; j < sizeof(IRData); j++) {
        writeEEPROM(tempAddress + 1 + sumRecordingLengths + 1 + j, writeBuffer[j]);

        #ifdef DEBUG_PRINTING
          Wire.end();
          serialPinInit();
          Serial.print("Writing ");
          Serial.print(writeBuffer[j], DEC);
          Serial.print(" to address ");
          Serial.println(tempAddress + 1 + sumRecordingLengths + 1 + j, DEC);
          Serial.flush();
          Serial.end();
          I2CPinInit();
        #endif
      }

      sumRecordingLengths += sizeof(IRData) + 1;
    }
  }
}


// Find the smallest address space where the bytes can fit
// TODO: Make it so that spaces can't be nagative and so that they can't overlap
unsigned int scanEmptyEEPROMAddresses(unsigned int bytesRequired) {
  unsigned int packetAddresses[NUMBER_OF_BUTTONS + 2];
  unsigned int packetLengths[NUMBER_OF_BUTTONS + 2];

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
    Serial.flush();
    Serial.end();
    I2CPinInit();
  #endif

  // Find the smallest space where the bytes will fit and return that address
  unsigned int shortestSpaceAddress = 0xFFFF;
  unsigned int addressSpace = 0;
  unsigned int bestAddress = 0;
  // Iterate through every button packet
  for (unsigned char i = 0; i < NUMBER_OF_BUTTONS + 1; i++) {
    addressSpace = packetAddresses[i+1] - (packetAddresses[i] + packetLengths[i]); // Save the address of the empty space
    
    // Compare the empty space, if it is large enough but also the smallest one discover yet, save it. 
    if (addressSpace >= bytesRequired && addressSpace < shortestSpaceAddress) {
      shortestSpaceAddress = addressSpace;
      bestAddress = packetAddresses[i] + packetLengths[i];
    }
  }
  
  return bestAddress;
}
