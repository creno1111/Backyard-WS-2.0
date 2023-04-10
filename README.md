## ESP32 Weather Station
WSv2 is a self contained weather station with a built in Web server. 

Hardware Sensing capabilities:
 - Temperature
 - Humidity
 - Barometric pressure
 - Wind direction
 - Wind speed

Data charting:

 - 1 hour charts of Temperature, Humidity, Barometric pressure and Wind speed.
 - 24 hour chars of Temperature, Humidity, Barometric pressure and Wind speed. 
 - 15 minute snapshots saved to internal FS, Space for a years worth of data.

NWS Weather lookup of current location. Zip code lookup for location change.

 - NWS Daily updates for the next 7 days
 - NWS Hourly updates for the next 24 hours
 - NWS Weather alerts

Windly.com storm radar, temp, rain, and wind radars for current location.

## Hardware

 - ESP32 Lolin32 1.0.0 (Picked for low power mode, will work with others esp32's)
 - BME280  (Amazon for 24.99)
 - AS5600 Magnetic encoder (Amazon 3 pack for 10.99)
 - 4mm brass tube (Amazon 6.99)
 - Some M3 screws or other fasteners of same size
 - Misc hardware for soldering
 - 3D printer and a 1kg of filament

## Software 
[NOTE: im a hobbyist, not a professional programmer, so please excuse my coding style, etc]

All code is Arduino compatible. PlatformIO is my platform of choice. Lots of things going on under the hood so plenty of libraries need to be included:
 * ├── ArduinoJson @ 6.21.1 (required: bblanchon/ArduinoJson @ ^6.21.1)
 * ├── BME280 @ 3.0.0 (required: finitespace/BME280 @ ^3.0.0)
 * ├── CircularBuffer @ 1.3.3 (required: rlogiacco/CircularBuffer @ ^1.3.3)
 * ├── ESP_WifiManager @ 1.10.2 (required: khoih-prog/ESP_WiFiManager @ ^1.3.0)
 * │   ├── ESP_DoubleResetDetector @ 1.3.1 (required: khoih-prog/ESP_DoubleResetDetector @ >=1.3.1)
 * │   │   └── LittleFS_esp32 @ 1.0.6 (required: lorol/LittleFS_esp32 @ ^1.0.6)
 * ├── ElegantOTA @ 2.2.9 (required: ayushsharma82/ElegantOTA @ ^2.2.7)
 * ├── NTPClient @ 3.2.1 (required: arduino-libraries/NTPClient @ ^3.1.0)
 * ├── WebSockets @ 2.3.7 (required: links2004/WebSockets @ ^2.3.6)
 * └── ezTime @ 0.8.3 (required: ropg/ezTime @ ^0.8.3)
 *  https://canvasjs.com/ (chart Javascript) for the local weather data charting

But if you use PlatformIO, I believe platform will help you get the libraries loaded. One library not listed that I had to manually add, the battery state monitor: BatteryRead, Copyright (c) 2019 Pangodream (MIT license).

[installing / first startup]
I will keep current firmware releases available for manual firmware uploads. Firmware releases can be programmed to the ESP32 using software, OTA or WSv2 uploader web page. 

Once Configuration portal is completed and WSv2 is running on the network, upload the files from the /src/data directory. Use the uploader webpage to upload the files, then select back or reboot. WSv2 should now be running as a Weather station.   

Or you can always clone this to your system and build it yourself. Think frustration free packages lol.

I usually get discouraged when I have to compile a project and debug it just to try it out. Hopefully the firmware file will help you evaluate without the headaches. It even detects a board without sensors and goes into test mode. In test mode some sensors have static values. So you can jump right into the projects weather station web UI. 

Note: First time sharing on GitHub