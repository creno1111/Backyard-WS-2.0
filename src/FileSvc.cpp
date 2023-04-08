#include "Arduino.h"
#include "FileSvc.h"
#include "Network.h"
#include "WebSvc.h"
#include "timekeeper.h"


extern FS* filesystem;
File file;
File fileSetting;
char fname[20];
char buffer[50];
extern float dayGraphTemp[];
extern int dayGraphHumidity[];
extern float dayGraphBarometric[];
extern float dayGraphWindspeed[];

/*****internal functions************/
//Check if file exists, Input month and day, creates filename global fname and checks if exists. Returns true or false
bool fileExists(int year, int month){  sprintf(fname,"/%i%02d.txt", year,month);  if(!FileFS.exists(fname)){ return false;  } return true; }
//fill sensor history data with null from hour to end of day 
void fillNullData(int hour){ hour*=4; while(hour < 96){ dayGraphTemp[hour] = 200; dayGraphHumidity[hour]=200; dayGraphBarometric[hour]=200; dayGraphWindspeed[hour]=200;  hour++;  }}
//fill sensor history data with saved data from start hour to end hour
void fillData(int readIndex, int hour) {
  if(hour < 0) hour = 0;
  while(readIndex < hour){
    if(file.seek(1,SeekCur)){
      int aa=0;
      while(aa < 4){
        file.readBytes(buffer,18);
        buffer[18]='\0'; 
        String dataPacket(buffer);
       // Serial.printf("ri: %i %s \n",readIndex, buffer);
        char strTmp[8];
          sprintf(strTmp,"%s", dataPacket.substring(0,5));    dayGraphTemp[(readIndex*4) + aa] = atof(strTmp) / 100.00;
          sprintf(strTmp,"%s", dataPacket.substring(5,8));    dayGraphHumidity[(readIndex*4) + aa] = atoi(strTmp) ;
          sprintf(strTmp,"%s", dataPacket.substring(8,13));   dayGraphBarometric[(readIndex*4) + aa] = atof(strTmp) / 100.00;
          sprintf(strTmp,"%s", dataPacket.substring(13,18));  dayGraphWindspeed[(readIndex*4) + aa] = atof(strTmp) / 100.00;
        aa++;
      } 
      readIndex++;
    } else {
      //Serial.println("here");
      fillNullData(readIndex);
      readIndex=24; 
    }
  }
}

void fileCleanup(void){ file.flush();  file.close(); }


/************ global functions **************/
//save hourly data to file, create file if missing. Fill data with 200 if timeslot info is missing
void saveHourlyData(void){ 
  timeOffset(3600, true);

  //find or create month file
  if(!fileExists(timeYear(),timeMonth())) { 
    Serial.printf("FS: Creating new month file %s\n",fname);
    file = FileFS.open(fname, "w"); 
    fileCleanup();
  }

  //find or create todays entry
  sprintf(buffer,"%02d:",timeDay());
  file = FileFS.open(fname, "r+"); 
  if(!file.find(buffer)){
    file.seek(0,SeekEnd);
    if(!file.position()==0){
     file.write('\n');
    }
    Serial.printf("FS: Creating new day record %s\n",buffer);
    int a=0; while(a!=3){ file.write(buffer[a]); a++; }
  }

  //transverse the day hours and record 200 for missing hours
  int hoursRecorded=0;
  while(file.find('~')){ hoursRecorded++; }
  while(hoursRecorded < timeHour()){
    int timeslot=0;  file.write('~');
    while(timeslot < 4){
      sprintf(buffer,"%05i%03i%05i%05i", 200*100,200,200*100,200*100);
      int a = 0;  while(a < 18){    file.write(buffer[a]);    a++;  }
      timeslot++;
    }
    hoursRecorded++;
  }
 // Serial.printf("pre HR... \n");
  //write out last hour of data
  int timeslot=0;  file.write('~');
  while(timeslot < 4){
    sprintf(buffer,"%05i%03i%05i%05i",(int)(dayGraphTemp[(timeHour() * 4) + timeslot] * 100), dayGraphHumidity[(timeHour() * 4) + timeslot],(int)(dayGraphBarometric[(timeHour() * 4) + timeslot] * 100), (int)(dayGraphWindspeed[(timeHour() * 4) + timeslot] *100));
    int a = 0;  while(a < 18){    file.write(buffer[a]);    a++;  }
    //Serial.printf("Hr TM SL: %s\n", buffer);
    timeslot++;
  }
  if(timeHour()==23) { file.write('\n'); }
  
  //cleanup
  fileCleanup();
  timeOffset(0,false);
}

/*******************************/
//read saved data, fill graph data with 200 if missing
//prefill = true, fill 24h of data
//prefill = false, fill up to hour
void readDataDay(int year, int month, int day, int hour, bool preFill){

  //check if file exists, write null data to buffer if missing.
  if(!fileExists(year,month)){
    fillNullData(hour);
    fileCleanup();
    return;
  }

  //check for day data, fill graph data with 200 if missing
  sprintf(buffer,"%02d:",day);
  file = FileFS.open(fname, "r"); 
  if(!file.find(buffer)){
    fillNullData(hour);
    fileCleanup();
    return;
  }

  //prefill data, retrive all data for the 24h period
  if(preFill){
    int readIndex =0;
    fillData(readIndex, 24);

  //prefill data, retrieve data to up to offset hour
  } else {
    int readIndex = 0;
    fillData(readIndex, hour);
  }
  fileCleanup();
}

//fill 24 hour buffers with saved data
void fill24hBuffer(void){
  if(timeHour()!=0){
    dateOffset(timeYear(),timeMonth(),timeDay() - 1);
    readDataDay(timeYear(),timeMonth(),timeDay(),timeHour(),true);
    dayGraphTemp[timeHour() *4] = 200; dayGraphTemp[timeHour() *4 +1] = 200;dayGraphTemp[timeHour() *4 +2] = 200;dayGraphTemp[timeHour() *4 +3] = 200;
    dayGraphHumidity[timeHour() *4] = 200; dayGraphHumidity[timeHour() *4 +1] = 200;dayGraphHumidity[timeHour() *4 +2] = 200;dayGraphHumidity[timeHour() *4 +3] = 200;
    dayGraphBarometric[timeHour() *4] = 200; dayGraphBarometric[timeHour() *4 +1] = 200;dayGraphBarometric[timeHour() *4 +2] = 200;dayGraphBarometric[timeHour() *4 +3] = 200;    
    dayGraphWindspeed[timeHour() *4] = 200; dayGraphWindspeed[timeHour() *4 +1] = 200;dayGraphWindspeed[timeHour() *4 +2] = 200;dayGraphWindspeed[timeHour() *4 +3] = 200;
    timeOffset(0,false);
    readDataDay(timeYear(),timeMonth(),timeDay(),timeHour(),false);
  } else {
    dateOffset(timeYear(),timeMonth(),timeDay() - 1);
    readDataDay(timeYear(),timeMonth(),timeDay(),timeHour(),true);
    dayGraphTemp[timeHour() *4] = 200; dayGraphTemp[timeHour() *4 +1] = 200;dayGraphTemp[timeHour() *4 +2] = 200;dayGraphTemp[timeHour() *4 +3] = 200;
    dayGraphHumidity[timeHour() *4] = 200; dayGraphHumidity[timeHour() *4 +1] = 200;dayGraphHumidity[timeHour() *4 +2] = 200;dayGraphHumidity[timeHour() *4 +3] = 200;
    dayGraphBarometric[timeHour() *4] = 200; dayGraphBarometric[timeHour() *4 +1] = 200;dayGraphBarometric[timeHour() *4 +2] = 200;dayGraphBarometric[timeHour() *4 +3] = 200;    
    dayGraphWindspeed[timeHour() *4] = 200; dayGraphWindspeed[timeHour() *4 +1] = 200;dayGraphWindspeed[timeHour() *4 +2] = 200;dayGraphWindspeed[timeHour() *4 +3] = 200;
    timeOffset(0,false);
  }
}

//check for settings file
const char settingsFile[] = "/settings.txt";

void fileCleanupSettings(void){ fileSetting.flush();  fileSetting.close(); }

bool fileExistsSettings(void){    if(!FileFS.exists(settingsFile)){ return false;  } return true;   }
 
//open settings file - read lat lon
void openSettings(double &lat, double &lon) {
  if(!fileExistsSettings()) { 
    Serial.printf("FS: Creating new settings.txt file\n");
    fileSetting = FileFS.open(settingsFile, "w"); 
    fileCleanupSettings();
  }
  fileSetting = FileFS.open(settingsFile, "r+"); 
  fileSetting.seek(0,SeekEnd);
    if(fileSetting.position()==0){
      Serial.println("here");
       int length =  sprintf(buffer,"lat:1.2345;lon:-5.4321;\n");
       int a = 0;  while(a < length){    fileSetting.write(buffer[a]);    a++;  }
       Serial.printf("length %i\n", length);
    }
  fileSetting.seek(0,SeekSet);
  int readSize = fileSetting.readBytes(buffer,40);
  buffer[readSize]='\0'; 
  String dataPacket(buffer);
  char strTmp[50];
  sprintf(strTmp,"%s", dataPacket.substring(dataPacket.indexOf(':')+1,dataPacket.indexOf(';')));  
  lat = atof(strTmp);
  sprintf(strTmp,"%s", dataPacket.substring(dataPacket.lastIndexOf(':')+1,dataPacket.lastIndexOf(';')));  
  lon = atof(strTmp);
  fileCleanupSettings();
}

//save settings file - save lat lon
void saveSettings(float lat, float lon){
  if(!fileExistsSettings()) { 
    Serial.printf("FS: Creating new settings.txt file\n");
    fileSetting = FileFS.open(settingsFile, "w"); 
    fileCleanupSettings();
  }
  fileSetting = FileFS.open(settingsFile, "r+"); 
  //Serial.printf("%.4f", lat);
  char strTmp[50];
  int length = sprintf(strTmp, "lat:%.4f;", lat);
  int a = 0;  while(a < length){    fileSetting.write(strTmp[a]);    a++;  }
      length = sprintf(strTmp, "lon:%.4f;\n", lon);
      a = 0;  while(a < length){    fileSetting.write(strTmp[a]);    a++;  }
      fileSetting.write(EOF);
  fileCleanupSettings();
}

  