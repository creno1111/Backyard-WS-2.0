#ifndef FILESVC_H
#define FILESVC_H

struct settingsWS {
  //Location data
  double lat;
  double lon;
  int zip;
  int DST;
  int tzOffset;
  //Sensor data
  int8_t WindDir;
  double WindOffset;
  double TempOffset;
  double BaroOffset;
  double HumidityOffset;
  //PWS data
  int WUUPD1;
  String WUURL1;
  String WUID1;
  String WUPW1;
  int WUUPD2;
  String WUURL2;
  String WUID2;
  String WUPW2;
  int WUUPD3;
  String WUURL3;
  String WUID3;
  String WUPW3;
  //misc data
  int BatDisp;
};

void saveHourlyData(void);
void readDataDay(int year, int month, int day, int hour, bool preFill);
void fill24hBuffer(void);
bool fileExistsSettings(void);
void openSettings(double &lat, double &lon);
settingsWS readSettings(void);
void writeSettings(void);

#endif //FILESVC_H