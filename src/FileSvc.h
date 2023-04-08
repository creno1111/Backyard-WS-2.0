#ifndef FILESVC_H
#define FILESVC_H


void saveHourlyData(void);
void readDataDay(int year, int month, int day, int hour, bool preFill);
void fill24hBuffer(void);
bool fileExistsSettings(void);
void openSettings(double &lat, double &lon);
void saveSettings(float lat, float lon);

#endif //FILESVC_H