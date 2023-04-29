# ESP32 Weather Station
WSv2 is a self contained weather station with a built in Web server. 
![alt text](https://github.com/creno1111/Backyard-WS-2.0/blob/master/img/WSv2%20Combined.png?raw=true)
(Rendering of current weatherstaion, along with a cut-away. Magnetic sensors and screws are missing from cut-away)

Hardware enclosure:
 - Fully 3D printed enclosure
 - Small brass rods with bearings for Wind Direction and Anemometer rotation
 - Screws and Epoxy in a couple places
 
Hardware Sensing capabilities:
 - Temperature
 - Humidity
 - Dew Point
 - Barometric pressure
 - Wind direction
 - Wind speed
 - Wind Gust

Data charting:

 - 1 hour charts of Temperature, Humidity, Barometric pressure and Wind speed.
 - 24 hour chars of Temperature, Humidity, Barometric pressure and Wind speed. 
 - 15 minute snapshots saved to internal FS, Space for a years worth of data.
 
 Weather Underground PWS protocol upload:
  - Upload weather data to 3 different sites that support WU PWS protocol 
  - Update intervals, URLs, IDs and Keys configured through the settings page

NWS (Nation weather service) of current location. Zip code lookup for location change.

 - NWS Daily updates for the next 7 days
 - NWS Hourly updates for the next 24 hours+
 - NWS Weather alerts

Windly.com storm radar/lightning, temp, rain, and wind radars for current location.

![alt text](https://github.com/creno1111/Backyard-WS-2.0/blob/master/img/WSv2%20Main%20web%20page.PNG?raw=true)
![alt text](https://github.com/creno1111/Backyard-WS-2.0/blob/master/img/WSv2%20Main%20web%20page%20(graphs).PNG?raw=true)

(Main web page compact and with live graphs, Internet Radar and NWS local forcast)

## Hardware

 - ESP32 Lolin32 1.0.0 (Picked for low power mode, will work with other esp32's)
 - BME280  (Amazon for $24.99)
 - AS5600 Magnetic encoder (Amazon 3 pack for $10.99)
 - 4mm brass tube (Amazon for $6.99)
 - Ball Bearing 5x8x2.5mm (Amazon 10 pack $9.99)
 - Some M3 screws or other fasteners of same size
 - Soldering station
 - 3D printer and filament

## Software 
[NOTE: im a hobbyist, not a professional programmer, so please excuse my coding style, etc]

All code is Arduino compatible. PlatformIO is my platform of choice. Lots of things going on under the hood so plenty of libraries need to be included:
 * ArduinoJson @ 6.21.1 (required: bblanchon/ArduinoJson @ ^6.21.1)
 * BME280 @ 3.0.0 (required: finitespace/BME280 @ ^3.0.0)
 * CircularBuffer @ 1.3.3 (required: rlogiacco/CircularBuffer @ ^1.3.3)
 * ESP_WifiManager @ 1.10.2 (required: khoih-prog/ESP_WiFiManager @ ^1.3.0)
 * ESP_DoubleResetDetector @ 1.3.1 (required: khoih-prog/ESP_DoubleResetDetector @ >=1.3.1)
 * LittleFS_esp32 @ 1.0.6 (required: lorol/LittleFS_esp32 @ ^1.0.6)
 * ElegantOTA @ 2.2.9 (required: ayushsharma82/ElegantOTA @ ^2.2.7)
 * NTPClient @ 3.2.1 (required: arduino-libraries/NTPClient @ ^3.1.0)
 * WebSockets @ 2.3.7 (required: links2004/WebSockets @ ^2.3.6)
 * ezTime @ 0.8.3 (required: ropg/ezTime @ ^0.8.3)
 * https://canvasjs.com/ (chart Javascript) for the local weather data charting
 * Chiper.cpp Created on: Feb 28, 2019 Author: joseph

But if you use PlatformIO, I believe platform will help you get the libraries loaded. One library not listed that I had to manually add, the battery state monitor: BatteryRead, Copyright (c) 2019 Pangodream (MIT license).

# Installing / first startup

Current firmware releases is available for manual firmware uploads. Firmware releases can be programmed to the ESP32 using software, OTA or WSv2 uploader web page if uploading from previous firmware. 

Once Configuration portal is completed and WSv2 is running on the network, upload the files from the /src/data directory. Use the uploader webpage to upload the files, then select back or reboot. WSv2 should now be running as a Weather station.  

After up and running, delete the settings.txt file if one exists using the Update page. Then go to the Settings page and save the information. This will create a new settings.txt from scratch, but more importantly it will generate a new Cipher key and save it in the EEPROM. Cipher keys are used to encrypt Keys/Passwords. Just repeat the delete of settings.txt if you ever want to regenerate another random Cipher key.
Note: the Cipher key isn't super secure at 16 bit AES, but it's better than having no encrption, and each device has it's own random private key.

![alt text](https://github.com/creno1111/Backyard-WS-2.0/blob/master/img/WSv2%20FS%20utility.PNG?raw=true)

(Built in FS utility to manage firmware, partition, files)



Note: First time sharing on GitHub, hope someone finds value from this project... more to come
