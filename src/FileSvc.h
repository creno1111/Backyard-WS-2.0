#ifndef FILESVC_H
#define FILESVC_H

struct settingsWS {
  double lat;
  double lon;
  int zip;
};

void saveHourlyData(void);
void readDataDay(int year, int month, int day, int hour, bool preFill);
void fill24hBuffer(void);
bool fileExistsSettings(void);
void openSettings(double &lat, double &lon);
settingsWS readSettings(void);
void saveSettings(float lat, float lon);
void writeSettings(void);

#endif //FILESVC_H