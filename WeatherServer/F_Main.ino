/************************************************************************************************************************************************
 *                                                             MAIN LOOP                                                                        *
 ************************************************************************************************************************************************                                                               
 */

/*
   Main Loop
   Wait for received to be true, meaning a sync stream plus
   all of the data bit edges have been found.
   Convert all of the pulse timings to bits and calculate
   the results.
*/
void loop() {
  
  ArduinoOTA.handle();                                        // for over the air updates
  
  server.handleClient();                                      // webserver

  if ( received == true )                                     // There's a messsage ready
  {

    unsigned int ringIndex;                                   // where in the bit buffer are we
    bool fail = false;                                        // flag to indicate good message or not
    byte dataBytes[bytesReceived];                            // Decoded message bytes go here  

    fail = false;                                             // reset bit decode error flag 

#ifdef DISPLAY_BIT_TIMING
    displayBitTiming();
#endif 

// ******************* Decode the message bit timings into bytes *******************************

    for ( int i = 0; i < bytesReceived; i++ )                 // clear the data bytes array
    {
      dataBytes[i] = 0;
    }

    ringIndex = (syncIndex + 1) % RING_BUFFER_SIZE;

    for ( int i = 0; i < bytesReceived * 8; i++ )             // Decode the timing into message bytes
    {
      int bit = convertTimingToBit( pulseDurations[ringIndex % RING_BUFFER_SIZE],
                                    pulseDurations[(ringIndex + 1) % RING_BUFFER_SIZE] );

      if ( bit < 0 )
      {
        fail = true;                                          // fails if any invalid bits
        break;                                                // exit loop
      }
      else
      {
        dataBytes[i / 8] |= bit << (7 - (i % 8));             // build the bytes
      }

      ringIndex += 2;
    }

#ifdef DebugOut
                                                            // Display the raw data received in hex
    if (fail)
    {
      Serial.println("Data Byte Display : Decoding error.");
    } else {
      for( int i = 0; i < bytesReceived; i++ )
      {
        Serial.print(dataBytes[i], HEX);
        Serial.print(",");
      }
    }
#endif

// *************** Validate the message via CRC and PARITY checks ******************************
    if (!fail) {
      fail = !acurite_parity(dataBytes, bytesReceived);      // check parity of each byte
    }

    if (!fail) {
      fail = !acurite_crc(dataBytes, bytesReceived);         // Check checksum of message
    }

// ********************* Determine which sensor it is from *************************************
    if (fail)  {

    } else if ((bytesReceived == 7)) {                
      decode_Acurite_6044(dataBytes);                 // Small Tower sensor with Temp and Humidity Only

    } else if ((bytesReceived == 8)) {                
      decode_5n1(dataBytes);                          //5n1 sensor

    } else if (bytesReceived == 9) {
       decode_Acurite_6045(dataBytes);                //Lightening detector

    } else {
      #ifdef DebugOut
      Serial.println("Unknown or Corrupt Message"); // shouldn't be possible, only 7, 8, or 9 byte messages should get here.
      #endif      
    }
    received = false;                                                                
  }                                                                        // Done with message, if we got one
    
  else {

    if ((millis() - Next5N1) > 20000) {                                    // check to see if we missed a message, we have if over 20 seconds since the last one

      Add5N1(false, CurrentWind, CurrentDirection, CurrentRain);           // missed the next 18.5 second update message
      Next5N1 = millis() - 1500;                                           // go back 1.5 seconds in time, since we were 1.5 seconds overdue 
      #ifdef DebugOut
      Serial.println("Missed 5n1"); 
      #endif
    }
    if ((millis() - NextTower) > 18000) {                                   
      AddTower(false);                                                      // missed the nex 16 second tower update
      NextTower = millis() - 2000;                                          // go back 2 seconds in time, since we were 2 seconds overdue   
      #ifdef DebugOut
      Serial.println("Missed Tower"); 
      #endif
    }
  }

  if ((millis() > midnight) && (midnight != 0)) {                         // time to reset rain fall?

    UpdateRainCounter();                                                  // reset daily rain fall by estimate that it is midnight

    midnight = millis() + (24 * 60 * 60 * 1000);                          // 24 hours * 60 minute/hour * 60 seconds/minute * 1000 milliseconds/second 

  }
                                                                          // update Weather Underground if it is time and if enough enough time before next message
                                                                          // delay WU update if tower or 5N1 message expected very soon
  if ((millis() > WUtime) && ((millis() - Next5N1) < 14000) && ((millis() - NextTower) < 12000) && (WUsend)) { 
                                                                          
    CurrentPressure = bmp.readPressure() * 0.000295333727;                // get the air pressure
    WUupdate = UpdateWeatherUnderground();                                // send data to Weather Underground
  }
  
  if ((millis()-displaylast) > 1000) { UpdateDisplay(); }                 // update display once a second

}

