/************************************************************************************************************************************************
 * UpdateWeatherUnderground()   : Routine to send weather data to Weather Underground, rapidfire                                                *
 * Returns true if WU accepted the data, false if anything went wrong                                                                           *
 ************************************************************************************************************************************************
 */

bool UpdateWeatherUnderground() {

  float avghumidity = 0;
  #ifdef DebugOut
  Serial.print("Updating WU, ");
  #endif

  WiFiClient client;                                                      // connect to Weather Underground
  if (!client.connect(WUserver, 80)) {

    WUresponse = "Connection Failed";                                     // Couldn't get a connection!
    WUtime = millis() + 60 * 1000;                                        // Try WU update again in 1 minute
    if (WUtime < millis()) { WUtime = 0; }                                // too close to rollover, wait for rollover
    #ifdef DebugOut
    Serial.println("Connection Fail");
    #endif
    return false;
  }

    client.print(WUpage);
    client.print("ID=");
    client.print(WUID);
    client.print("&PASSWORD=");
    client.print(WUpassword);
    client.print("&dateutc=");
    client.print("now");    

    client.print("&baromin=");                                            // air pressure from internal sensor
    client.print(CurrentPressure,2);
  
  if (Signal5N1 > 4) {                                                    // Send 5N1 data if available
    
    if (CurrentWind > 1) {                                                // is there enough wind to send a wind direction?
      client.print("&winddir=");
      client.print(winddirections[CurrentDirection],1);
    }
    client.print("&windspeedmph=");
    client.print(CurrentWind,1);
    if (WindSpeedPeak > 1) {
      client.print("&windgustmph_10=");                                     // 10 minute peak, they don't seem to do anything with this
      client.print(WindSpeedPeak,1);
      client.print("&windgustdir_10m=");                            
      client.print(winddirections[WindPeakDirection],1);
    }
    client.print("&windspdmph_avg2m=");
    client.print(WindSpeedAverage2m,1);
    if (WindSpeedPeak2m > 1) {
      client.print("&windgustmph=");                                        // they don't really say how long for gust, use 2 minute peak
      client.print(WindSpeedPeak2m,1);
      client.print("&windgustdir=");
      client.print(winddirections[WindPeakDirection2m],1);
    }
    if ((CurrentRain >= 0) && (RainRate >=0)) {
      client.print("&rainin=");
      client.print(RainRate,2);
      client.print("&dailyrainin=");
      client.print(CurrentRain,2);
    }
  }

  if ((SignalTower > 5) && (TemperatureTower > -40) && (HumidityTower > 0) &&
      (Signal5N1 > 5) && (Temperature5N1 > -40) && (Humidity5N1 > 0)) {                   // average them if getting both
    client.print("&tempf=");
    client.print((TemperatureTower + Temperature5N1)/2,1);
    client.print("&humidity=");
    avghumidity = (HumidityTower + Humidity5N1)/2;
    client.print(avghumidity,0);
    client.print("&dewptf=");
    client.print(DewPointF((TemperatureTower + Temperature5N1)/2,avghumidity));
  } else if ((SignalTower > 2) && (TemperatureTower > -40) && (HumidityTower > 0)) {      // still getting tower readings?
    client.print("&tempf=");                                                              // from the Tower
    client.print(TemperatureTower,1);
    client.print("&humidity=");
    client.print(HumidityTower,DEC);
    client.print("&dewptf=");
    client.print(DewPointF(TemperatureTower,HumidityTower));
  } else if ((Signal5N1 > 2) && (Temperature5N1 > -40) && (Humidity5N1 > 0)) {            // still getting 5n1 readings?
    client.print("&tempf=");                                                              // from the 5n1 thsn
    client.print(Temperature5N1,1);
    client.print("&humidity=");
    client.print(Humidity5N1,DEC);
    client.print("&dewptf=");
    client.print(DewPointF(Temperature5N1,Humidity5N1));
  }

  client.print("&softwaretype=Custom%20version1.3&action=updateraw&realtime=1&rtfreq=37");    // finish up
  client.println("/ HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n");             

  WUresponse = "";
  while(client.connected()) {                                                               // get the response
    while (client.available()) {
      WUresponse += char(client.read());
      }   
  }
  client.flush();
  client.stop();

  #ifdef DebugOut
  Serial.print("WU Response: ");
  Serial.println(WUresponse);
  #endif

  if (WUresponse.indexOf("success") > -1) {

    WUtime = millis() + 37 * 1000;                                        // WU update again in 37 seconds
    if (WUtime < millis()) { WUtime = 0; }                                // too close to rollover, wait for rollover
    return true;

  } else {

    WUtime = millis() + 10 * 60 * 1000;                                   // Try WU update again in 10 minutes
    if (WUtime < millis()) { WUtime = 0; }                                // too close to rollover, wait for rollover
    return false;
  }


}




/************************************************************************************************************************************************
 ************************************************************************************************************************************************
 **                                                         Webpages                                                                           **
 ************************************************************************************************************************************************
 ************************************************************************************************************************************************
 */


/************************************************************************************************************************************************
 *                                                          Root                                                                                *
 * Just identify ourselves                                                                                                                      *                                                         
 ************************************************************************************************************************************************
 */

void handleRoot() {

  server.send ( 200, "text/plain", "Personal Weather Server\r\n\ndebug = current debug data\r\nDATA = weather data for HomeSeer\r\nRESET = midnight reset of rain fall" );

}


/************************************************************************************************************************************************
 *                                                          debug                                                                               *
 * Provide debug data                                                                                                                           *                                                         
 ************************************************************************************************************************************************
 */

void handleDebug() {

  float DTime = 0;
  float Level433 = 0;
 
  String debugstr ="Weather Server Debug Page \r\nVersion: ";              // system
  debugstr += Version;
  debugstr += "\r\n";
  DTime = millis()/(10 * 60 * 60);
  DTime = DTime/100;                                                      // do it this way to get 1/100th, otherwise comes as an integer
  debugstr += String(DTime,2);
  
  debugstr += " hours uptime\r\n\nWiFi signal level = ";                  // Radio levels
  debugstr += String(WiFi.RSSI());

  debugstr += " dBm, 433MHz receiver signal level = ";                    // Crude approximation from the MICRF211 data sheet                     
  Level433 = (((analogRead(A0) - 74)*80)/641) - 120;                      // note that the AD is apparently 0 to 3.2 Volt input range (default)
  Level433 = constrain(Level433, -120, -40);                              // 0V = 0, 3.2V = 1024
  debugstr += String(Level433,0);
  
  debugstr += " dBm\r\n\nTime to next WU update = ";                      // Weather Underground
  DTime = (WUtime - millis())/1000;
  if (WUtime > millis()) {
    debugstr += String(DTime,1);
  } else {
    debugstr += "Overdue";
  }
  debugstr += " seconds, Last WU response:\r\n";
  debugstr += WUresponse;

  debugstr += "\r\n\nAir Pressure = ";
  debugstr += String(CurrentPressure,2);                                  // Air pressure
  debugstr += " inHg, ";
  debugstr += String(bmp.readPressure());
  debugstr += " Pa, Receiver Temperature = ";
  debugstr += String((bmp.readTemperature()* 9 / 5) +32,1);

  debugstr += " F\r\n\n5-in-1 Sensor:\r\nETA of next message: ";          // 5-in-1
  DTime = (Next5N1 + 18500 - millis())/1000;
  debugstr += String(DTime,0);
  debugstr += " seconds, Last good message was ";
  DTime = (millis()-LastGood5N1)/1000;
  debugstr += String(DTime,0);
  debugstr += " seconds ago, Signal quality = ";
  debugstr += String(Signal5N1,DEC);  
  debugstr += " out of 10\r\nTemperature = ";
  debugstr += String(Temperature5N1,1);
  debugstr += " F, Relative Humidity = ";
  debugstr += String(Humidity5N1,DEC);
  debugstr += "%\r\nDaily Rain = ";
  debugstr += String(CurrentRain,2);
  debugstr += " inches, Current Rain Rate = ";
  debugstr += String(RainRate,2);
  debugstr += " inches/hour, Rain Counter = ";
  debugstr += String(RainCounter5N1,HEX);
  debugstr += "\r\nWind = ";
  debugstr += String(CurrentWind,1);
  debugstr += " MPH, Direction = ";
  debugstr += acurite_5n1_winddirection_str[CurrentDirection];
  debugstr += ", Average = ";
  debugstr += String(WindSpeedAverage,1);
  debugstr += " MPH, Peak = ";
  debugstr += String(WindSpeedPeak,1);
  debugstr += " MPH at ";
  debugstr += acurite_5n1_winddirection_str[WindPeakDirection];
  if (BatteryLow5N1 == true) {
    debugstr += " Direction\r\nBattery Low\r\n\n";
  } else {
    debugstr += " Direction\r\n\n";
  }
  
  debugstr += "Tower Sensor:\r\nETA of next message: ";          // Tower
  DTime = (NextTower + 18500 - millis())/1000;
  debugstr += String(DTime,0);
  debugstr += " seconds, Last good message was ";
  DTime = (millis()-LastGoodTower)/1000;
  debugstr += String(DTime,0);
  debugstr += " seconds ago, Signal quality = ";
  debugstr += String(SignalTower,DEC);  
  debugstr += " out of 10\r\nTemperature = ";
  debugstr += String(TemperatureTower,1);
  debugstr += " F, Relative Humidity = ";
  debugstr += String(HumidityTower,DEC);
  if (BatteryLowTower == true) {
    debugstr += "%\r\nBattery Low\r\n\nReading#, 5N1 Signal, Tower Signal, Rain, Wind Speed, Direction\r\n";
  } else {
    debugstr += "%\r\n\nReading#, 5N1 Signal, Tower Signal, Rain, Wind Speed, Direction\r\n";
  }
  
  for ( int i = 32; i > 0; i-- ) {                                      // signal, wind, and rain array
    debugstr += String(i,DEC);
    if (i < 10) {
      debugstr += " ";
    }
    if (Mess5N1[i] == true) {
      debugstr += "         Good       ";  
    } else {
      debugstr += "         Missed     ";
    }
    
    if (i < 11) {                                                           // only have ten message flags for tower sensor
      if (MessTower[i] == true) {
        debugstr += "Good          ";  
      } else {
        debugstr += "Missed        ";
      }
    } else {
      debugstr += "              ";  
    }
    
    debugstr += String(Rain[i],2);
    debugstr += "  ";
    debugstr += String(WindSpeed[i],2);
    debugstr += "        ";
    debugstr += acurite_5n1_winddirection_str[WindDirection[i]];
    debugstr += "\r\n";
  }
  debugstr += "\r\n";

  server.send ( 200, "text/plain", debugstr );

}


/************************************************************************************************************************************************
 *                                                          DATA                                                                                *
 * Give Weather data to HomeSeer.  Data is:                                                                                                     *                                                         
 * Signal5N1, Temperature5N1, Humidity5N1, CurrentWind, WindSpeedAverage, WindSpeedPeak, CurrentRain, RainRate, CurrentPressure,        *
 * BatteryLow5N1, BatteryLowTower                                                                                                               *
 ************************************************************************************************************************************************
 */

void handleData() {

  CurrentPressure = bmp.readPressure() * 0.000295333727;                // get the air pressure
  String Wdata ="";  

  Wdata += String(Signal5N1,DEC);                                        // Signal quality from 0 to 10
  Wdata += ",";

  Wdata += String(Temperature5N1,1);                                  // Temperature from - 40.0 to 140.0 in degrees F from the 5n1
  Wdata += ",";
  Wdata += String(Humidity5N1,DEC);                                  // Relative humidity from 0 to 99 % from the 5n1
  Wdata += ",";

  Wdata += String((WindSpeed[32]+WindSpeed[31]+WindSpeed[30]+WindSpeed[29])/4,1); // Current wind speed from 0.0 to 98.8 MPH
  Wdata += ",";

  Wdata += String(WindSpeedAverage,1);                                    // Average wind speed from 0.0 to 98.8 MPH
  Wdata += ",";

  Wdata += String(WindSpeedPeak,0);                                       // Peak wind speed from 0 to 98.8 MPH
  Wdata += ",";

  Wdata += String(CurrentRain,2);                                         // Current daily rain fall amount from 0.00 to ??.?? inches
  Wdata += ",";

  Wdata += String(RainRate,2);                                            // Current daily rain fall amount from 0.00 to ??.?? inches
  Wdata += ",";

  Wdata += String(CurrentPressure,2);                                     // Current air pressure in inHg
  Wdata += ",";

  if (BatteryLow5N1 == true) {                                            // AcuRite 5N1 low battery flag
     Wdata += "Low,";   
   } else {
     Wdata += "Ok,";
   }

  if (BatteryLowTower == true) {                                          // AcuRite Tower low battery flag
     Wdata += "Low,";   
   } else {
     Wdata += "Ok,";
   }

  Wdata += String(SignalTower,DEC);                                       // Acurite Tower Signal quality

  Wdata += ",";
  Wdata += String(TemperatureTower,1);                                    // Temperature from the tower

  Wdata += ",";
  Wdata += String(HumidityTower,DEC);                                  // Relative humidity from 0 to 99 % from the tower

  Wdata += ",END";

  server.send ( 200, "text/plain", Wdata );
  
}


/************************************************************************************************************************************************
 *                                                          RESET                                                                               *
 * Reset daily rainfall, assume it is midnight                                                                                                  *                                                         
 ************************************************************************************************************************************************
 */

void handleReset() {

  String Wdata ="";  

  Wdata = " Today's total rain was ";
  Wdata += String(CurrentRain,2);
  Wdata += " inches.";
  
  UpdateRainCounter();                                          // reset daily rain fall
  
  midnight = millis() + (24 * 60 * 60 * 1000) + 5000;           // 24 hours * 60 minute/hour * 60 seconds/minute * 1000 milliseconds/second 
                                                                // add 5 seconds to avoid reset before next reset command.
                                                                
  server.send ( 200, "text/plain", Wdata );                     // today's (yesterday's really) total rain fall

}


/************************************************************************************************************************************************
 *                                                    Requested page doesn't exist                                                              *
  ************************************************************************************************************************************************
 */

void handleNotFound() {

  server.send ( 404, "text/plain", "Page Not Found" );

}

