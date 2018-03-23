/**********************************************************************
   Arduino code decode several Acurite devices OTA data stream.

   Modified 3/21/2018 Joe Roberts


   Decoding protocol and prototype code from these sources:
   Ray Wang (Rayshobby LLC)
     http://rayshobby.net/?p=8998
   Benjamin Larsson (RTL_433)
     https://github.com/merbanan/rtl_433
   Brad Hunting (Acurite_00592TX_sniffer)
     https://github.com/bhunting/Acurite_00592TX_sniffer

   Written and tested on an Arduino Uno R3 Compatible Board
   using a RXB6 433Mhz Superheterodyne Wireless Receiver Module

   This works with these devices but more could be added
     Acurite Pro 5-in-1 (8 bytes)
      https://tinyurl.com/zwsl9oj
     Acurite Ligtning Detector 06045M  (9 bytes)
      https://tinyurl.com/y7of6dq4
     Acurite Room Temp and Humidity 06044M (7 bytes)
      https://tinyurl.com/yc9fpx8q
*/

#define TestBuild                                     // uncomment for the test build instead of actual unit, also turns on debug output
//#define DebugOut                                      // uncomment to stream data out serial port
//#define DISPLAY_BIT_TIMING                            // uncomment to show debug bit timings

/************************************************************************************************************************************************
 *                                                            INCLUDES                                                                          *
 ************************************************************************************************************************************************                                                           
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include <Wire.h>                                       // I2C

#include <Adafruit_Sensor.h>                            
#include <Adafruit_BMP280.h>                            // air pressure & temperature sensor

#include <Adafruit_GFX.h>                               // Display
#include <Adafruit_SSD1306.h>                           // Specific OLED display

#include <WiFiUdp.h>                                    // for OTA updates
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

/************************************************************************************************************************************************
 *                                                            DEFINES                                                                           *
 ************************************************************************************************************************************************                                                           
 */

#define OLED_RESET 4                                  // For I2C OLED display

#define BaudRate  115200                              // baud rate for serial debug data  --- should set as high as possible instead of 115200


#define RING_BUFFER_SIZE  152                         // ring buffer size has to be large enough to fit data and sync signal

#define SYNC_HIGH       610                           // nominal data and sync bit sizes in msec  ** was 600
#define SYNC_LOW        610

#define PULSE_LONG      400
#define PULSE_SHORT     210                           // ** was 220

#define PULSE_RANGE     100                           // tolerance on sync and data pulses

#define BIT1_HIGH_MIN  (PULSE_LONG-PULSE_RANGE)       //BIT1
#define BIT1_HIGH_MAX  (PULSE_LONG+PULSE_RANGE)
#define BIT1_LOW_MIN   (PULSE_SHORT-PULSE_RANGE)
#define BIT1_LOW_MAX   (PULSE_SHORT+PULSE_RANGE)

#define BIT0_HIGH_MIN  (PULSE_SHORT-PULSE_RANGE)      //BIT0
#define BIT0_HIGH_MAX  (PULSE_SHORT+PULSE_RANGE)
#define BIT0_LOW_MIN   (PULSE_LONG-PULSE_RANGE)
#define BIT0_LOW_MAX   (PULSE_LONG+PULSE_RANGE)

#define SYNCPULSECNT    (4)                           // 4 pulses (8 edges)
#define SYNCPULSEEDGES  (SYNCPULSECNT*2)

#define DATABYTESCNT_MIN (7)                          // Minimum number of data bytes
#define DATABITSCNT_MIN     (DATABYTESCNT_MIN*8)      // 7 bytes * 8 bits
#define DATABITSEDGES_MIN   (DATABITSCNT_MIN*2)

#define DATABYTESCNT_MID 128                          // 8 Bytes

#define DATABYTESCNT_MAX (9)                          // 9 Bytes
#define DATABITSCNT_MAX     (DATABYTESCNT_MAX*8)
#define DATABITSEDGES_MAX   (DATABITSCNT_MAX*2)


#define DATAPIN         (D1)                           // radio data pin, must use pin that can interrupt on any level change

                                                       // 5n1 Tower Message Types
#define  MT_WS_WD_RF  49                               // wind speed, wind direction, rainfall
#define  MT_WS_T_RH   56                               // wind speed, temp, RH




/************************************************************************************************************************************************
 *                                                       Global Constants                                                                       *
 ************************************************************************************************************************************************                                                           
 */

char  Version[] = "Build 3/21/2018 A";

const char* ssid = "Your SSID";                                           // local WiFI 
const char* password = "Your Password";

#ifdef TestBuild
  char HostName[] = "wserver-test";                                     // test build unit name
#else
  char HostName[] = "wserver";                                          // actual build name
#endif

//char WUserver [] = "weatherstation.wunderground.com";                 // normal
char WUserver [] = "rtupdate.wunderground.com";                         // rapid fire
char WUpage [] = "GET /weatherstation/updateweatherstation.php?";
char WUID [] = "Your station ID";
char WUpassword [] = "Your password";

#ifdef TestBuild
  bool WUsend = false;                                                  // don't send to WU if this is a test unit
#else
  bool WUsend = true;                                                   // true to send to WU
#endif

int ID5N1 = 0x0000;                                                     // 5N1 ID, set to 0 to allow any 5N1
int IDTower = 0x0000;                                                   // Tower ID, set to 0 to allow any tower

const float winddirections[] = { 315.0, 247.5, 292.5, 270.0,      // wind directions in degrees for WU for 5-N-1
                                 337.5, 225.0, 0.0, 202.5,
                                 67.5, 135.0, 90.0, 112.5,
                                 45.0, 157.5, 22.5, 180.0
                               };

char * acurite_5n1_winddirection_str[] =                          // Wind directions in standard terms, not really needed **
{ "NW",  // 0  315
  "WSW", // 1  247.5
  "WNW", // 2  292.5
  "W",   // 3  270
  "NNW", // 4  337.5
  "SW",  // 5  225
  "N",   // 6  0
  "SSW", // 7  202.5
  "ENE", // 8  67.5
  "SE",  // 9  135
  "E",   // 10 90
  "ESE", // 11 112.5
  "NE",  // 12 45
  "SSE", // 13 157.5
  "NNE", // 14 22.5
  "S"    // 15 180
};  


/************************************************************************************************************************************************
 *                                                       Global Variables                                                                       *
 ************************************************************************************************************************************************                                                           
 */

                                                      // The pulse durations are the measured time in milliseconds between pulse edges
unsigned long IpulseDurations[RING_BUFFER_SIZE];                                                                                   
//unsigned int IsyncIndex  = 0;                       // index of the last bit time of the sync signal                            
unsigned int IchangeCount = 0;                                                                                                     

unsigned long pulseDurations[RING_BUFFER_SIZE];                                                                                   
unsigned int syncIndex  = 0;                          // index of the last bit time of the sync signal                            
bool         received  = false;                       // true if sync plus enough bits found                                      
unsigned int bytesReceived = 0;                                                                                                   





int RainCounter5N1 = 0;                     // 5-n-1 rain counter, refreshed from eeprom
int CurrentRainCounter5N1 = 0;
float RainYesterday = 0;

bool Mess5N1[33] = {false, false, false, false, false, false, false, false, false, false, false,          // good message flags for last 33 messages
                    false, false, false, false, false, false, false, false, false, false, false,
                    false, false, false, false, false, false, false, false, false, false, false};
float WindSpeed[33] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   // table of last 33 wind speed readings
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float Rain[33] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                       // table of last 33 rain fall readings
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int WindDirection[33];
                                                                      
float WindSpeedAverage = 0;                                                                               // average wind speed (over 10 minutes)
float WindSpeedAverage2m = 0;                                                                             // average wind speed (over 2 minutes)
float WindSpeedPeak = 0;                                                                                  // Peak wind speed (over 10 minutes)
int WindPeakDirection = 0;                                                                                // wind direction of 10 minute peak
float WindSpeedPeak2m = 0;                                                                                // Peak wind speed (over 2 minutes)
int WindPeakDirection2m = 0;                                                                              // wind direction of 2 minute peak
float RainRate = 0;                                                                                       // Predicted rainfall rate (inchs/hour)
unsigned long Next5N1 = 0;                                                                                // track time between messages from 5n1
unsigned long LastGood5N1 = 0;                                                                            // time of last good 5N1 message received
int Signal5N1 = 0;                                                                                        // Signal quality (0 = none, 10 = full) based on 3 minutes of messages
bool BatteryLow5N1 = false;                                                                               // flag for low battery for 5-n-1
float CurrentRain = 0;                                                                                    // current amount of rain today
float Temperature5N1 = -999;
float CurrentWind = 0;
int CurrentDirection = 0;
int Humidity5N1 = 0;
float CurrentPressure = 0;
unsigned long midnight = 0;                           // estimated time of next midnight, in milliseconds


float TemperatureTower = -999;
int HumidityTower = 0;
bool BatteryLowTower = false;
int SignalTower = 0;
bool MessTower[11] = {false, false, false, false, false, false, false, false, false, false, false};        // good message flags for last 11 messages
unsigned long NextTower = 0;                          // timer between messages from the tower temp/humidty sensor
unsigned long LastGoodTower = 0;                      // time of last good tower message received

int strikeTot = 0;                                    // Lightning sensor stuff
int strikeWrapOffset = 0;
int lastStrikeCount = 0;
unsigned long lightninglast = 0;                      // timer between messages from the lightning detector


unsigned long WUtime = 1000 * 60 * 5;                 // time to send next Weather Underground message. normally every 37 seconds, but wait 5 minutes to begin with
String WUresponse = "No send attempt yet";
bool WUupdate = false;                                // was the last update to Weather Underground successful?

unsigned long displaylast = 0;



/************************************************************************************************************************************************
 *                                                    Library Stuff                                                                             *
 ************************************************************************************************************************************************                                                           
 */

Adafruit_SSD1306 display(OLED_RESET);                 // OLED display

Adafruit_BMP280 bmp;                                  // I2C Pressure Sensor

ESP8266WebServer server ( 80 );                       // Web stuff



#ifdef TestBuild
  #define DebugOut
#endif
