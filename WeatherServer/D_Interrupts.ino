/************************************************************************************************************************************************
 *                                                  Interrupt Handler for IO                                                                    *
 *  Fires everytime the data signal from the 433MHz radio changes state                                                                         *                                                
 *  Finds and stores sync and data bits                                                                                                         *
 *  Once it finds sync bits and enough data bits, it sets the recieved flag to true and stops processing the signal until                       *
 *  received is set to false again.  Failure to set received to false quickly, will result in lost messages.                                    *
 * **********************************************************************************************************************************************
 */

/* Interrupt 1 handler
   Tied to pin 3 INT1 of arduino.
   Set to interrupt on edge (level change) high or low transition.
   Change the state of the Arduino LED (pin 13) on each interrupt.
   This allows scoping pin 13 to see the interrupt / data pulse train.
*/
void handler()
{
  static unsigned long duration = 0;
  static unsigned long lastTime = 0;
  static unsigned int ringIndex = 0;
  static unsigned int syncCount = 0;
  static unsigned int bitState  = 0;
  static unsigned int IsyncIndex  = 0;                                    // was global                            
  static bool         IsyncFound = false;                                 // true if sync pulses found 
  static unsigned int IbytesReceived = 0;                                                                                                   
 
  bitState = digitalRead(DATAPIN);
  digitalWrite(LED_BUILTIN, !bitState);                                   // have LED mimic state of RF signal

  long time = micros();                                                   // calculating timing since last change
  duration = time - lastTime;
  lastTime = time;

                                                                          // Known error in bit stream is runt/short pulses.
                                                                          // If we ever get a really short, or really long,
                                                                          // pulse we know there is an error in the bit stream
                                                                          // and should start over.

  if ( (duration > (PULSE_LONG + PULSE_RANGE)) || (duration < (PULSE_SHORT - PULSE_RANGE)) )
  {
    if (IsyncFound  && IchangeCount >= DATABITSEDGES_MIN)                //Check to see if we have received a minimum number of bits we could take
    {
      if (IchangeCount >= DATABYTESCNT_MID)
      {
        IbytesReceived = 8;                                             // 5-N-1
      } else {
        IbytesReceived = 7;                                             // tower
      }
      if (!received) {                                                  // lost this message if main wasn't finished with the last one
        received = true;                                                // for the main loop
        syncIndex = IsyncIndex;
        bytesReceived = IbytesReceived;
        for ( int i = 0; i < RING_BUFFER_SIZE; i++ ) {
          pulseDurations[i] = IpulseDurations[i];
        }
      }
      IsyncFound = false;
      return;

    } else {
      IsyncFound = false;
      IchangeCount = 0;                                                  // restart looking for data bits
    }
  }

  ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;                       // store data in ring buffer
  IpulseDurations[ringIndex] = duration;
  IchangeCount++;                                                       // found another edge

  if ( isSync(ringIndex) )                                              // detect sync signal
  {
    IsyncFound = true;
    IchangeCount = 0;                                                   // restart looking for data bits
    IsyncIndex = ringIndex;
  }

  if ( IsyncFound )                                                     // If a sync has been found the start looking for the data bit edges.
  {
    if ( IchangeCount < DATABITSEDGES_MAX ) {                           // if not enough bits yet, no message received yet
    }
    else if ( IchangeCount > DATABITSEDGES_MAX ) {
    
      IsyncFound = false;
    }
/*    else if (IchangeCount >= DATABYTESCNT_MID) {
      
      IbytesReceived = 8;                                             // 5-n-1
      if (!received) {                                                // lost this message if main wasn't finished with the last one
        received = true;                                              // for the main loop
        syncIndex = IsyncIndex;
        bytesReceived = IbytesReceived;
        for ( int i = 0; i < RING_BUFFER_SIZE; i++ ) {
          pulseDurations[i] = IpulseDurations[i];
        }
      }
      IsyncFound = false;
      return;

    } */
    else
    {                                                                 
      IbytesReceived = 9;                                             // need to find better way to terminate receive !!!!!!!!!  Probably use message ID to know how many bytes to get
      if (!received) {                                                // lost this message if main wasn't finished with the last one
        received = true;                                              // for the main loop
        syncIndex = IsyncIndex;
        bytesReceived = IbytesReceived;
        for ( int i = 0; i < RING_BUFFER_SIZE; i++ ) {
          pulseDurations[i] = IpulseDurations[i];
        }
      }
      IsyncFound = false;
      return;

    }
  }
}

