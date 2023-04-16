#ifndef SENSOR_H
#define SENSOR_H

#include "Wire.h"
inline TwoWire I2C_1 = TwoWire(0);
inline TwoWire I2C_2 = TwoWire(1);
inline bool secRotarySensor = false;
inline bool netSvc = false;

void setOffsets(void);
int batteryRead(void);
bool sensorsInit(void);
float readTemp(void);
int readHumidity(void);
void sensorsSvc(void);
float readPressure(void);
float readWindSpeed(void);
String readWindDirection(void);
float readHeatIndex(void);

#endif //SENSOR_H