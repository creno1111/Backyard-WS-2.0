This project reads: Humidity, Temperature, Wind speed / direction, Barometric pressure. 

EPS32 as the Weather Sensor and Dynamic WebServer / WebPage 
1 Sensor: temp, pressure, humidity
1 Magnetic sensor configured in analog mode (wind speed)
1 Mangetic sensor configured in SPI more (wind direction)
1 12v to 5v buck regulator
Custom 3D printed housing, all-in-one design

*********

 First time using GitHub. Figured I should share, wanted a good weather station in my back yard.
 I already have 12V from my little backyard solar setup. Didn't want to pay $hundreads for a 
 complete weather station. And here we are.

PlatformIO is platform of choice. 
All hardware is tested and already in use for months now. 3D prints are holding up to the elements.

Will be uploading 3D prints, and some documentation on the ESP32 and electronics hookup. 

 <More to come...>

 From src/main.cpp

 /************************************************************
 * project: Weather Station V2.0
 * 2nd iteration of Arduino based, comon parts plus 3d Printed enclosre.
 *  * WSv1: First weather station was sepperated into different sensor packages
 *  and a RaspberryPI was used w/graphana as a dispaly.
 *  * WSv2 [CURRENT], This version is all-in-one enclosure with embedded ESP32
 *  webserver serving up a dynamic evenromental weather page with history
 *  and graphs.
 *  
 * Creator: CReno
 * Date: 2021-01-01 - present
 * [NOTE: im a hobbyist, not a professional programmer, so please excuse my coding style, etc]
 * 
 * esp32 based, common off the shelf parts (arduino based)
 * 
 * FEATURES:
 * humidity, temperature, barometric pressure, wind speed, wind direction
 *  History graphing, OTA updates, config portal, NTP time sync
 *  WiFi, WebSocket, WebServer, JSON, SPI, OTA, NTP, MDNS,
 *  DNS, AP, TCP, UDP, HTTP, SPIFFS, GPIO, PWM, ADC
 * monthly data FS loggin file updated every 15 minutes,
 *  minute data display, 24 hour data display
 *  every 0.01-3 seconds sensors are recorded(ram)
 * NSW weather warnings, weather forecast, hourly data
 * Windy.com built in Lightning, Radar, Temperature, Rain Acc, Wind
 * Built-in FS(FW) utility. OTA(FW,Partition, FS File),
 *  FS List,Size,Upload,Download 
 * 3D printed enclosure 
 * 
 * 
 * TODO:
 * - add ability to reset WS data from web, detect new setup and initialize also
 * - better wifi forced reset (currently set reset, compile upload, disable reset, compile upload)
 * - add upload to GitHub
 * - add zip code save
 * - add zip code lookup validation
 * - add bootstrap to html
 * - add ability for windy displays to auto size
 * - add proper attribution to all borrowed code, library's, etc
 * - add settings page, manual upd of location, and future settings
 * - add ability to integrate wunderground API
 * - add wireless ESP32 epaper weather display project (tbd)
 * - add page to display FS historic data
 * - add FS low space detection and auto FS history cleanup(fifo)
 * - fix NSW Hourly icons - sometimes not loading
 * - change HTML indicator text for humidity, temp, barometric, windspeed, etc
 * - change HTML wind speed from float to int, .00 to .99 is kinda random in low winds
 * - misc code cleanup, ugh - im a lazy coder :)
 * - add 3D files to the repository
 * 
 * Libraries
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
 *  https://canvasjs.com/ (chart Javascript) for test weather data charting in servced webpage
*/
