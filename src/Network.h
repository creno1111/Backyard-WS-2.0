
#ifndef Network_h
#define Network_h

#include "arduino.h"
#include <SPIFFS.h>


void heartBeatPrint(void);
void WiFiHealthCheck(void);
void check_WiFi(void);
void check_status(void);
void loadConfigData(void);
void saveConfigData(void);
uint8_t connectMultiWiFi(void);
void wifiSetup(void);

void setupNTP(void);
int getHours(void);
int getMinutes(void);
int getSeconds(void);
void NTPEvents(void);
byte displayIP(void);
String FSData(void);
int getStrength(int points);
bool getLatLon(long zip, double &lat, double &lon);
bool wUndergroundPWS(void);

#endif //Network_h