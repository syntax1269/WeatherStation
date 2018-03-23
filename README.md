My changes:
Fixed some decode bugs, such as the low battery flag for the 5-N-1
Added parity check and crc to the messages
Changed project to run on a NodeMCU (WiFi Arduino)
Added Weather Underground Upload of data
Added some webpages to get data into my home automation system (probably not much use to anyone else)
Added a display and a presure sensor
Added rain rate calculation, wind peak, and wind average
Added signal quality score
Put rain counter in eeprom, so that resets don't mess up the rain amount per day

I only have a 5-N-1 and a tower sensor.  I average the temps and humidity from both to send to WU, you may want to do this differently
I do not have a lightning sensor, so I didn't really do anything with it, and I don't know if what is there works

The big thing that it still needs is a way to tell time for the midnight resets for the rain amounts.  You could add NTP for time/date,
or you could get it from the response from WU.  Correcting for time zone is easy, correcting for DST is a pain.

Sample from debug webpage:

Weather Server Debug Page 
Version: Build 3/20/2018 B
37.98 hours uptime

WiFi signal level = -41 dBm, 433MHz receiver signal level = -68 dBm

Time to next WU update = 34.0 seconds, Last WU response:
HTTP/1.0 200 OK
Content-type: text/html
Date: Fri, 23 Mar 2018 16:46:00 GMT
Content-Length: 8

success


Air Pressure = 30.34 inHg, 102725.80 Pa, Receiver Temperature = 90.1 F

5-in-1 Sensor:
ETA of next message:  1 seconds, Last good message was 16 seconds ago, Signal quality = 10 out of 10
Temperature =68.9 F, Relative Humidity = 65%
Daily Rain = 0.00 inches, Current Rain Rate = 0.00 inches/hour, Rain Counter = 10d
Wind = 2.7 MPH, Direction = SE, Average = 5.4 MPH, Peak = 9.4 MPH at SSW Direction

Tower Sensor:
ETA of next message:  5 seconds, Last good message was 13 seconds ago, Signal quality = 9 out of 10
Temperature =68.2 F, Relative Humidity = 55%

Reading#, 5N1 Signal, Tower Signal, Rain, Wind Speed, Direction
32         Good                     0.00  2.68        SE
31         Good                     0.00  6.28        SE
30         Good                     0.00  6.28        SE
29         Good                     0.00  2.68        SE
28         Good                     0.00  4.74        S
27         Good                     0.00  9.37        S
26         Good                     0.00  6.28        SSE
25         Good                     0.00  2.68        SSE
24         Good                     0.00  3.71        SSW
23         Good                     0.00  7.31        SSW
22         Good                     0.00  5.77        SSW
21         Good                     0.00  5.77        SSW
20         Good                     0.00  6.28        SSW
19         Good                     0.00  7.82        SSW
18         Good                     0.00  8.34        SSW
17         Good                     0.00  5.77        SSW
16         Good                     0.00  3.71        S
15         Good                     0.00  4.22        S
14         Good                     0.00  8.34        SSW
13         Good                     0.00  6.79        SSW
12         Good                     0.00  3.19        SSE
11         Missed                   0.00  5.77        SSE
10         Good       Good          0.00  5.77        SSE
9          Good       Good          0.00  6.79        SSE
8          Good       Good          0.00  6.28        SSW
7          Good       Good          0.00  5.77        SSW
6          Good       Good          0.00  9.37        SSW
5          Good       Good          0.00  3.19        SSW
4          Good       Good          0.00  6.28        SSW
3          Good       Good          0.00  3.19        SSW
2          Good       Missed        0.00  2.16        E
1          Good       Good          0.00  1.65        E




Old readme:

Arduino code decode several Acurite devices OTA data stream.
  
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
 
