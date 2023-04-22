#include "Arduino.h"
#include "FileSvc.h"
#include "Network.h"
#include "WebSvc.h"
#include "timekeeper.h"
#include "eeprom.h"

#include "cipher.h"

extern FS* filesystem;
File file;
File fileSetting;
char fname[20];
char buffer[50];
extern float dayGraphTemp[];
extern int dayGraphHumidity[];
extern float dayGraphBarometric[];
extern float dayGraphWindspeed[];
extern struct settingsWS settings_WS;

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

//close file system properly
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


/************ Settings file read / write *******************/
//Settings file global (TODO: shoudl be local, will fix later)
const char settingsFile[] = "/settings.txt";

//cleanup after use, settings FS object
void fileCleanupSettings(void){ fileSetting.flush();  fileSetting.close(); }

//check for settings file, TODO: should be integrated to the other file checks, integrate later
bool fileExistsSettings(void){    if(!FileFS.exists(settingsFile)){ return false;  } return true;   }

//cipher helpers, create key, retrieve key from eeprom, read and write ciphered data
void cipherKeyNew(void){
    Serial.printf("************** Creating new cyrpto key ************************");
    EEPROM.begin(512);
    String key = "";
    randomSeed(analogRead(33)); // initialize random seed
    for (int i = 0; i < 16; i++) {
      key += (char)(random(65, 122)); // add a random character to the string
    }
    Serial.printf("\nNew key: %s\n", key.c_str());
    EEPROM.writeBytes(0, key.c_str(), 17);
    EEPROM.commit();
}

char * cipherKeyRead(void){ char *key = new char[17];
  EEPROM.begin(512);
  EEPROM.readBytes(0, key, 16);
  key[16] = '\0';
  return key;
}

String readCipher(String hidden){
  Cipher *cipher = new Cipher();
  cipher->setKey(cipherKeyRead());
  return cipher->decryptString(hidden);
}

String writeCipher(String hide){
  Cipher *cipher = new Cipher();
  cipher->setKey(cipherKeyRead());
  return cipher->encryptString(hide);
}

//read settings file, return settings struct
settingsWS readSettings(void){
  if(!fileExistsSettings()) { 
    cipherKeyNew();
    fileSetting = FileFS.open(settingsFile, "w"); 
    fileCleanupSettings();
  }
  fileSetting = FileFS.open(settingsFile, "r+"); 
  while (fileSetting.available()) {
    String line = fileSetting.readStringUntil('\n');
    if(line[0] == '#'){ continue; } //skip reading, comment line
    int colonIndex = line.indexOf(':');
    if (colonIndex >= 0) {
      String token = line.substring(0,colonIndex);
      String result = line.substring(colonIndex + 1);
      if(token == "lat"){        settings_WS.lat = result.toFloat();} 
      else if(token == "lon"){   settings_WS.lon = result.toFloat();} 
      else if(token == "zip"){   settings_WS.zip = result.toInt();  }
      else if(token == "DST"){   settings_WS.DST = result.toInt();  }
      else if(token == "WindDir"){   settings_WS.WindDir = result.toInt();  }
      else if(token == "TmpOffset"){   settings_WS.TempOffset = result.toFloat();  }
      else if(token == "HumOffset"){   settings_WS.HumidityOffset = result.toFloat();  }
      else if(token == "BarOffset"){   settings_WS.BaroOffset = result.toFloat();  }
      else if(token == "WindOffset"){   settings_WS.WindOffset = result.toFloat();  }
      else if(token == "WUUPD"){   settings_WS.WUUPD = result.toInt(); }
      else if(token == "WUID"){   settings_WS.WUID = result; }
      else if(token == "WUPW"){   settings_WS.WUPW = readCipher(result); }
      else if(token == "BatDisp"){   settings_WS.BatDisp = result.toInt(); }
    }
  }
  fileCleanupSettings();
  return settings_WS;
}

//write setting to settings file
void writeSettings(void){
  if(!fileExistsSettings()) { 
    fileSetting = FileFS.open(settingsFile, "w"); 
    cipherKeyNew();
    fileCleanupSettings();
  }
  fileSetting = FileFS.open(settingsFile, "w"); // Open the settings file for writing
  if (fileSetting) {
    fileSetting.printf("# lat, lon & zip are updated after NWS zip lookup (webpage)\n");
    fileSetting.printf("lat:%f\n", settings_WS.lat); // Write the latitude value
    fileSetting.printf("lon:%f\n", settings_WS.lon); // Write the longitude value
    fileSetting.printf("zip:%i\n", settings_WS.zip); // Write the zip code value
    fileSetting.printf("\n# Other settings are set from the settings page\n");
    fileSetting.printf("DST:%i\n", settings_WS.DST);
    fileSetting.printf("WindDir:%i\n", settings_WS.WindDir);
    fileSetting.printf("TmpOffset:%f\n", settings_WS.TempOffset); 
    fileSetting.printf("HumOffset:%f\n", settings_WS.HumidityOffset);
    fileSetting.printf("BarOffset:%f\n", settings_WS.BaroOffset); 
    fileSetting.printf("WindOffset:%f\n", settings_WS.WindOffset); 
    fileSetting.printf("WUUPD:%i\n", settings_WS.WUUPD); 
    fileSetting.printf("WUID:%s\n", settings_WS.WUID); 
    fileSetting.printf("WUPW:%s\n", writeCipher(settings_WS.WUPW).c_str());
    fileSetting.printf("BatDisp:%i\n", settings_WS.BatDisp);
    fileSetting.printf("\n# To obtain a new cipher key, delete the settings file.\n");
    fileSetting.printf("# On save of settings page the cipher key will be randomly regenerated and saved to the EEPROM.\n");
    fileSetting.printf("# On save of settings page a new settings file will be written.\n");
    fileSetting.printf("\n# End of file");
    fileCleanupSettings();
  }
}
  