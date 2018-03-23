/************************************************************************************************************************************************
 ************************************************************************************************************************************************
 **                                                           Functions                                                                        **
 ************************************************************************************************************************************************
 ************************************************************************************************************************************************
 */

/************************************************************************************************************************************************
 * isSync(idx) : Function to look for start of message signal                                                                                   *
 * idx is the index (pointer) to the last measured signal bit.  Uses global array variable pulseDurations, and global variable SYNCPULSEEDGES   *
 * Returns TRUE if start of message found, FALSE if not.                                                                                        *
 *                                                                                                                                              *
 * Searches the pulseDurations array backwards looking for two pairs of long high long low pulses.  (long is 600 msec +/- 125 msec              *
 * **********************************************************************************************************************************************
 */
 
bool isSync(unsigned int idx)
{
  for ( int i = 0; i < SYNCPULSEEDGES; i += 2 )                                          // check if we've received 4 pulses of matching timing
  {
    unsigned long t1 = IpulseDurations[(idx + RING_BUFFER_SIZE - i) % RING_BUFFER_SIZE];
    unsigned long t0 = IpulseDurations[(idx + RING_BUFFER_SIZE - i - 1) % RING_BUFFER_SIZE];

    if ( t0 < (SYNC_HIGH - PULSE_RANGE) || t0 > (SYNC_HIGH + PULSE_RANGE) ||            // any of the preceeding 8 pulses are out of bounds, short or long,
         t1 < (SYNC_LOW - PULSE_RANGE)  || t1 > (SYNC_LOW + PULSE_RANGE) )               // return false, no sync found
    {
      return false;
    }
  }
  return true;
}


/************************************************************************************************************************************************
 * convertTimingToBit(t0,t1)  : Function to identify pulse pair as 0, 1, or undefined                                                           *
 * t0 is the length of the first pulse, and t1 is the length of the second pulse, length is in milliseconds                                     *
 * Returns 0 for short high followed by long low, 1 for long high followed by short low, -1 otherwise                                           *
 *                                                                                                                                              *
 * long is 400 msec +/- 125 msec, short is 220 msec +/- 125 msec                                                                                *
 ************************************************************************************************************************************************
 */

int convertTimingToBit(unsigned int t0, unsigned int t1)
{
  if ( t0 > (BIT1_HIGH_MIN) && t0 < (BIT1_HIGH_MAX) && t1 > (BIT1_LOW_MIN) && t1 < (BIT1_LOW_MAX) )
  {
    return 1;                                                                                               // "1" bit
  }
  else if ( t0 > (BIT0_HIGH_MIN) && t0 < (BIT0_HIGH_MAX) && t1 > (BIT0_LOW_MIN) && t1 < (BIT0_LOW_MAX) )
  {
    return 0;                                                                                               // "0" bit
  }
  return -1;                                                                                                 // undefined or sync
}


/************************************************************************************************************************************************
 * acurite_crc(row[],bytes)   : Function to verify the CRC of the message                                                                       *
 * row[] is the array of bytes to check. bytes is the number of bytes in the message                                                            *
 * Returns TRUE if the message CRC is good, FALSE if it is not                                                                                  *
 *                                                                                                                                              *
 * The checksum is a simple modulo-256 sum of all the bytes of the message except the last.  The last is the checksum to match                  *
 * **********************************************************************************************************************************************
 */

bool acurite_crc(volatile byte row[], int bytes) {                          // Validate the CRC value to validate the packet

  int sum = 0;                                                              // sum of first n-1 bytes modulo 256 should equal nth byte

  for (int i = 0; i < bytes - 1; i++) {
    sum += row[i];
  }
  if ((sum != 0) && ((sum & 0xFF) == row[bytes - 1])) {
    return true;                                                            // validated
  } else {
    
#ifdef DebugOut
    Serial.print("CRC ERROR: ");
    Serial.println(sum % 256, HEX);
#endif

    return false;                                                           // failed
  }
}
    
  
/************************************************************************************************************************************************
 * Parity(MessByte)   : Function to check the parity of a byte                                                                                  *                                                                                 
 * MessByte is the 8 bit byte to check parity on.                                                                                               *
 * Returns TRUE if the byte has even parity, FALSE if it does not                                                                               *
 *                                                                                                                                              *
 * Note that the first two bytes and the last byte of each message do not have parity (true for 5-in-1, true for others?                        *
 * **********************************************************************************************************************************************
 */

bool Parity(int MessByte) {

  int bitSum = 0;
  int b = 1;                                              // start with bit 0, will end at bit 7

  for (int i = 0; i<8; i++) {                             // check all eight bits
    if (MessByte & (b << i)) {
      bitSum++;
    }
  }
  if ((bitSum % 2) == 0) {                                // is it even (no remainder when divided by two)
    return true;
  } else {
    return false;
  }
}


/************************************************************************************************************************************************
 * acurite_parity(row[],bytes)   : Function to verify the CRC of the message                                                                    *
 * row[] is the array of bytes to check. bytes is the number of bytes in the message                                                            *
 * Returns TRUE if the parity of all bytes except the first two and the last one is good, FALSE if not                                          *
 * **********************************************************************************************************************************************
 */

bool acurite_parity(volatile byte row[], int bytes) {                          // Validate the parity of each byte in the packet

      for( int i = 2; i < bytes - 1; i++ ) {                                // Skip the first two and the last byte
        if (Parity(row[i]) ==  false){
          #ifdef DebugOut
          Serial.print("Parity Error: ");
          Serial.print(row[i], HEX);
          Serial.println();
          #endif
          return false;                                                    // exit loop if parity error found
        }
      }
      return true;
}


/************************************************************************************************************************************************
 * acurite_6045_getTemp (highbyte,lowbyte)    : Function to decode temperature reading from 6045 lightning sensor                               *
 * highbyte is byte 4 of the message, and lowbyte is byte 5 of the message.                                                                     *
 * Returns the temperature in Fahrenhelt, -40.0 F to 158.0 F                                                                                    *
 * **********************************************************************************************************************************************
 */

float acurite_6045_getTemp (uint8_t highbyte, uint8_t lowbyte) {
  int rawtemp = ((highbyte & 0x1F) << 7) | (lowbyte & 0x7F);
  float temp = (rawtemp - 1500) / 10.0;
  return temp;
}


/************************************************************************************************************************************************
 * acurite_getTemp_6044M(highbyte,lowbyte)    : Function to decode temperature reading from 6044 tower sensor                                   *
 * hibyte is byte 4 of the message, and lobyte is byte 5 of the message.                                                                        *
 * Returns the temperature in Fahrenhelt, -40.0 F to 158.0 F                                                                                    *
 * **********************************************************************************************************************************************
 */

float acurite_getTemp_6044M(byte hibyte, byte lobyte) {                               

  int highbits = (hibyte & 0x0F) << 7;
  int lowbits = lobyte & 0x7F;
  int rawtemp = highbits | lowbits;
  float temp = (rawtemp / 10.0) - 100;
  return temp;
}


/************************************************************************************************************************************************
 * acurite_getTemp_5n1(highbyte,lowbyte)    : Function to decode temperature reading from 5-n-1 sensor                                          *
 * hibyte is byte 4 of the message, and lobyte is byte 5 of the message.                                                                        *
 * Returns the temperature in Fahrenhelt, -40.0 F to 158.0 F                                                                                    *
 * **********************************************************************************************************************************************
 */

float acurite_getTemp_5n1(byte highbyte, byte lowbyte) {                          

  int highbits = (highbyte & 0x0F) << 7 ;
  int lowbits = lowbyte & 0x7F;
  int rawtemp = highbits | lowbits;
  float temp_F = (rawtemp - 400) / 10.0;
  return temp_F;
}


/************************************************************************************************************************************************
 * convCF(c)    : Function to convert a temperature reading in Celsius to Fahrenheit                                                            *
 * c is the temperature reading in Celsius                                                                                                      *
 * Returns the temperature reading in Fahrenheit                                                                                                *
 ************************************************************************************************************************************************
 */

float convCF(float c) {
  return ((c * 1.8) + 32);
}


/************************************************************************************************************************************************
 * acurite_6045_strikeCnt(strikeByte)   : Function to get the number of lightning strikes detected today by the 6045 lightning sensor           *
 * strikeByte is the 6th byte from the message.  Uses several global variables                                                                  *
 * Returns the number of strikes (today)                                                                                                        *
 *                                                                                                                                              *
 * Strike count is reset at midnight (elsewhere)                                                                                                *
 ************************************************************************************************************************************************
 */

int acurite_6045_strikeCnt(byte strikeByte){
  
  int strikeCnt = strikeByte & 0x7f;
  if (strikeTot == 0)
  {
    strikeTot = strikeCnt;                                          //Initialize Strike Counter
    strikeWrapOffset = 0;
    return 0;
  } else if (strikeCnt < strikeTot && strikeCnt > 0) {            // Did strike counter on sensor wrap?         
    strikeWrapOffset = (127 - strikeTot) + strikeWrapOffset;
    strikeTot = 1;                                                //*Strikes wrapped around, must set to 1 instead of 0 because of this
  }
  return (strikeCnt - strikeTot) + strikeWrapOffset;

}


/************************************************************************************************************************************************
 * acurite_6045_strikeRange(strikeRange)    : Function to decode the distance of the last lightning strike detected by the 6045                 *
 * strikeRange is byte 7 of the message                                                                                                         *   
 * Returns the estimated distance of the last lightning strike in miles                                                                         *
 ************************************************************************************************************************************************
 */

uint8_t acurite_6045_strikeRange(uint8_t strikeRange)
{
  return strikeRange & 0x1f;
}


/************************************************************************************************************************************************
 * acurite_txr_getSensorID(hibyte,lobyte)   : Function to get the ID of the wireless sensor that transmitted the message                        *
 * hibyte is byte 0 of the message and lobyte is byte 1 of the message                                                                          *
 * Returns the sensor ID as 16 bits  (only 14 bits used)                                                                                        *
 ************************************************************************************************************************************************
 */

uint16_t acurite_txr_getSensorId(uint8_t hibyte, uint8_t lobyte) {
  return ((hibyte & 0x0f) << 8) | lobyte;
}



/************************************************************************************************************************************************
 * acurite_getHumidity(byte)    : Function to decode the humidity reading from the sensor                                                       *
 * byte is byte 6 of the 5-n-1 message, byte 3 of the tower and lightning detector messages                                                     *
 * Returns the relative humidity percentage as an integer ranging from 1 to 99                                                                  *
 ************************************************************************************************************************************************
 */

int acurite_getHumidity (uint8_t byte) {
  int humidity = byte & 0x7F;                                                     
  return humidity;
}


/************************************************************************************************************************************************
 * acurite_getWindSpeed_kph(highbyte, lowbyte)    : Function to decode the wind speed from the sensor                                           *
 * highbyte is byte 3 and lowbyte is byte 4 of the message from the 5-n-1.  Byte ? and byte ?+1 of the 3-n-1?                                   *
 * Returns the current (peak of last 18 seconds?) wind speed in km/hour                                                                         *
 ************************************************************************************************************************************************
 */
 
float acurite_getWindSpeed_kph (uint8_t highbyte, uint8_t lowbyte) {         // range: 0 to 159 kph

  int highbits = ( highbyte & 0x1F) << 3;                                    // raw number is cup rotations per 4 seconds
  int lowbits = ( lowbyte & 0x70 ) >> 4;                                     // http://www.wxforum.net/index.php?topic=27244.0 (found from weewx driver)
  int rawspeed = highbits | lowbits;
  float speed_kph = 0;
  if (rawspeed > 0) {
    speed_kph = rawspeed * 0.8278 + 1.0;
  }
  return speed_kph;
}


/************************************************************************************************************************************************
 * UpdateRainCounter()    : Function to update the rain counter                                                                                 *
 ************************************************************************************************************************************************
 */
void UpdateRainCounter() {

  if (RainCounter5N1 != CurrentRainCounter5N1) {
    RainCounter5N1 = CurrentRainCounter5N1;                                 // reset rain counter to current value                            
    EEPROM.write(0,highByte(RainCounter5N1));
    EEPROM.write(1,lowByte(RainCounter5N1));
    EEPROM.commit();
    
    RainYesterday = CurrentRain;
    CurrentRain = 0;                                                        // reset daily rain (would reset on next rain message received anyway

    #ifdef DebugOut
    Serial.print(" *Rain Counter set to ");
    Serial.print(RainCounter5N1,HEX);
    Serial.print("* ");
    #endif
  }
}


/************************************************************************************************************************************************
 * convKphMph(kph)    : Function to convert a wind speed in km/hour to miles/hour                                                               *
 * kph is the windspeed in km per hour                                                                                                          *
 * Returns the wind speed in miles per hour                                                                                                     *
 ************************************************************************************************************************************************
 */

float convKphMph(float kph) {
  return kph * 0.62137;
}


/************************************************************************************************************************************************
 * getWindDirection_Descr(byte)   : Function to decode the wind direction from the 5-n-1 in standard (terse) direction terms                    *
 * byte = byte 4 of 5-n-1 message, or byte ? of 3-n-1 message                                                                                   *
 * Returns the direction as a string such as N, W, E, S, NW, SSE, ...                                                                           *
 ************************************************************************************************************************************************
 

//char * getWindDirection_Descr(byte b) {
//  int direction = b & 0x0F;
//  return acurite_5n1_winddirection_str[direction];
//}
*/

/************************************************************************************************************************************************
 * acurite_getRainfall(hibyte,lobyte)   : Funtion to decode the current rainfall amount from the 5-n-1 message                                  *
 * hibyte is byte 5 and lobyte is byte 6 of the message.  Uses several global variables                                                         *
 * Returns today's rainfall amount in inches.  Is precise to 1/100th of an inch                                                                 *
 *                                                                                                                                              *
 * Need to reset to zero at midnight (elsewhere)                                                                                                *                                                                                                                                             
 ************************************************************************************************************************************************
 */
float acurite_getRainfall(uint8_t hibyte, uint8_t lobyte) {                     // range: 0 to 99.99 in, 0.01 in incr., rolling counter?

  CurrentRainCounter5N1 = ((hibyte & 0x3f) << 7) | (lobyte & 0x7F);             // 5N1's just received rain counter value, 0 to 0x1FFF           

                                                                                // Was the EEPROM never written?  Default is 0xFFFF
                                                                                // Over 0x1FFF is not possible
                                                                                // Did 5N1 flag battery change (so counter changed)
  if (RainCounter5N1 > 0x1FFF) {                                                // Should we reset the raincounter ?
    UpdateRainCounter();
    return (0);  
  }

  if ((RainRate == 0) && (RainCounter5N1 < 0x1FD0) && (CurrentRainCounter5N1 == 0)) {    // rain counter resets to zero when
                                                                                            // batteries are changed in the 5N1
    UpdateRainCounter();
    return (0);
  }
  
  if (CurrentRainCounter5N1 >= RainCounter5N1) {                                // did the counter wrap around?
    return ((CurrentRainCounter5N1 - RainCounter5N1) * 0.01);                   // No, it didn't
  } else {
    return ((CurrentRainCounter5N1 - (RainCounter5N1 - 0x2000)) * 0.01);        // Yes, it did
  }
}


/************************************************************************************************************************************************
 * DewPointF(Temperature,Humidity)    : Function to calculate appoximate dew point in Fahrenheit                                                *
 * Temperature is in Fahrenheit, Humidity is 0 to 100 relative humidity                                                                         *
 * Returns the dew point temperature in Fahrenheit                                                                                              *
 ************************************************************************************************************************************************
 */

float DewPointF(float tempf, int humidity) {

  float A0= 373.15/(273.15 + tempf);
  float SUM = -7.90298 * (A0-1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
  SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
  SUM += log10(1013.246);
  float VP = pow(10, SUM-3) * humidity;
  float T = log(VP/0.61078);   
  return (241.88 * T) / (17.558-T);
  
//  return ( tempf - ( 9 * (100 - humidity) / 25));   // the simple way
}


/************************************************************************************************************************************************
 * GetLetter(byte0)   :  Function to return the letter value of the 3 position switch on the sensor                                             *
 * byte0 is the first byte of the message                                                                                                       *
 * Returns A, B, or C for the switch position.  Returns E if there is an error                                                                  *
 ************************************************************************************************************************************************
 */
char GetLetter(int byte0) {

  static char chLetter[4] = {'C','E','B','A'}; 
  return chLetter[(byte0 & 0xC0) >> 6];
}


/************************************************************************************************************************************************
 * GetMessageNo(byte0)    : Function to return the message number of the 5N1 message                                                            *
 * byte0 is the first byte of the message                                                                                                       *
 * Returns 1, 2, or 3, is it the 1st, 2nd, or 3rd of each repeated message?                                                                     *
 ************************************************************************************************************************************************
 */
int GetMessageNo(int byte0) {
  return ((byte0 & 0x30) >> 4);
}

