#ifndef FILESVC_H
#define FILESVC_H

struct settingsWS {
  double lat;
  double lon;
  int zip;
  int DST;
  int8_t WindDir;
  double WindOffset;
  double TempOffset;
  double BaroOffset;
  double HumidityOffset;
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