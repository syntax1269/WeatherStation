/************************************************************************************************************************************************
 *                                                               Setup                                                                          *
 ************************************************************************************************************************************************                                                               
 */

void setup()
{
  Serial.begin(BaudRate);
  #ifdef DebugOut                                                        // stream data out serial for debug
  Serial.println();
  Serial.println("Acurite Receive Started...");
  #endif
  
/*************************************************
 *** January 12, 2021, 11:38:48 AM »
 *** Some library changes have effected the code for the BMP280.  
 *** If you want to use a BMP280 with this, you need to find and comment out the following lines:
 *** 
 *** digitalWrite(LED_BUILTIN, !bitState);    - in D_Interrupts
 *** 
 *** digitalWrite(LED_BUILTIN, HIGH); // start with LED off   - E_Setup
 *** pinMode(LED_BUILTIN, OUTPUT); // LED output   - E_Setup
 ***********************************************/
  
  
  pinMode(LED_BUILTIN, OUTPUT);                                           // LED output
  digitalWrite(LED_BUILTIN, HIGH);                                        // start with LED off
  
  Wire.begin(D3, D4);                                                     // I2C  (SDA, SCL)
  Wire.setClock(100000);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);                              // initialize OLED with the I2C addr 0x3C (for the 128x32), turn on display charge pump
  display.clearDisplay();                                                 // Display is 128 x 32 dots, 21 characters x 4 lines
  display.setTextSize(2);                                                 // Size 2 is 10 characters by 2 lines
  display.setTextWrap(false);                                             // Don't jump to the next line
  display.setTextColor(WHITE,BLACK);                                      // Over write the background with each character
  display.setCursor(16,8);                                                // Start kind of centered
  display.println("STARTING");
  display.display();

  bmp.begin();                                                            // initialize pressure sensor

  #ifdef DebugOut                                                         // stream data out serial for debug
  Serial.println();                               
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif

  EEPROM.begin(32);                                                       // Need two bytes in EEPROM 
  RainCounter5N1 = (EEPROM.read(0) << 8) | EEPROM.read(1);                // recover rain counter from EEPROM
  CurrentRainCounter5N1 = RainCounter5N1;
  #ifdef DebugOut
  Serial.print("Rain Counter loaded from EEPROM is ");
  Serial.println(RainCounter5N1,HEX);
  #endif

  WiFi.mode ( WIFI_STA );
  WiFi.begin(ssid, password);                                             // connect to the WiFi
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);                                                           // This needed to give control back to the esp to run its own code!  
    #ifdef DebugOut                                                       // stream data out serial for debug
    Serial.print(".");
    #endif
  }

  #ifdef DebugOut                                                          
  Serial.println("");
  Serial.println("WiFi connected");
  #endif
  
  pinMode(DATAPIN, INPUT);                                                // 433MHz radio data input
  attachInterrupt(digitalPinToInterrupt(DATAPIN), handler, CHANGE);       // interrupt on radio data signal

  display.clearDisplay();                                                 // Start over 
  display.setTextSize(1);                                                 // text is 6 dots x 8 dots
  display.setCursor(0,0); 
  display.print(WiFi.localIP());                                          // Printing the ESP IP address
  display.setCursor(128-(5*6),0);                                         // top line, 5 characters from the end
  display.print("WU:");
  display.setCursor(0,12);                                                // Using 3 lines where 4 would fit, so spread out a bit
  display.print("5N1   S:00  T:9999999");                                 // prefil status lines
  display.setCursor(0,24); //   654321
  display.print("Tower S:00  T:9999999");
              // 123456789012345678901
  display.display();

  #ifdef DebugOut                                                           
  Serial.println(WiFi.localIP());
  #endif

  server.on ( "/", handleRoot );                                          // root webpage, just identify the device

  server.on ( "/debug",handleDebug );                                     // show info for possible debugging

  server.on ( "/DATA",handleData );                                       // give weather data to HomeSeer

  server.on ( "/RESET",handleReset );                                     // midnight signal to reset daily rain amount

  server.onNotFound ( handleNotFound );

  server.begin();

                                                                          // OTA stuff
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HostName);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else                                                      // U_SPIFFS
      type = "filesystem";
                                                              // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    display.clearDisplay();                                                 
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("OTA");
    display.setTextSize(1);
    display.setCursor(6*2*3+6,0);
    display.print("Update");
    display.setCursor(6*2*3+6,8);
    display.print(type);
    display.display();
    display.setCursor(0,8*2);
    display.setTextSize(1);
    display.print("Progress: ");
    detachInterrupt(digitalPinToInterrupt(DATAPIN));
  });
  ArduinoOTA.onEnd([]() {
    display.setCursor(0,8*2);
    display.setTextSize(2);
    display.print("END        ");
    display.display();
    detachInterrupt(digitalPinToInterrupt(DATAPIN));
    delay(1000);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.setCursor(60,8*2);
    display.setTextSize(1);
    display.print((progress / (total / 100)));
    display.display();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    display.setTextSize(1);
    display.setCursor(0,24);
    if (error == OTA_AUTH_ERROR) display.print("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) display.print("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) display.print("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) display.print("Receive Failed");
    else if (error == OTA_END_ERROR) display.print("End Failed");
    display.display();
    delay(5000);
  });
  ArduinoOTA.begin();
}


