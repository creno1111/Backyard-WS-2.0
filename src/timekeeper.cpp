#include "Arduino.h"
#include "timekeeper.h"
#include "NTPClient.h"
#include "time.h"
#include "Network.h"
#include "WebSvc.h"
#include "fileSvc.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org", 0, 60000);
struct tm ts;
struct tm tsOffset;
long offsetSet=0;
bool offsetEn=false;
time_t epoc;

extern struct settingsWS settings_WS;

//initialize NTP services and set time offset
void timeNTPStart(void){
    timeClient.begin();
    timeClient.setTimeOffset(-18000); //offset for EST time
}


//Check for time sync. true if synced, false if not
bool timeCheck(void){ 
//added for debugging purposes, related var usage in the timecheck() routine
    static bool timeSet = false;
    static long NTPsyncDly = 0;
        static int setHour = 99;
    static int setMinute = 99;
    if(millis() > NTPsyncDly){
        NTPsyncDly = millis() + 60000;
        int time = 0;
        if(!timeClient.update() && time!=3) {
            timeClient.forceUpdate();
            time++;
        }  
        if(time==3){ Serial.printf("No NTP sync after %i tries.\n", time); timeSet=false;  }  //failed time check
        timeSet=true;
    }
    int epocDST = 0;
    if(settings_WS.DST == 0) epocDST = 3600;  //DST offset
    epoc = timeClient.getEpochTime() + epocDST;
    ts = *localtime(&epoc);
    if(setMinute!=99){
        ts.tm_min = setMinute;
    }
    if(setHour!=99){
        ts.tm_hour = setHour;
    }
    if(timeYear() < 2020) timeSet = false;          //time sanity check
    return timeSet;                                 //Current time, or unreliable
}

//offset epoc time. 3600 minutes is 1 day in the past, enable will enable the offset
void timeOffset(long minutes,bool enable){
    if(enable) offsetEn = true;
    else offsetEn = false;
    int epocDST = 0;
    if(settings_WS.DST == 0) epocDST = 3600;  //DST offset    
    epoc = timeClient.getEpochTime() - minutes + epocDST;; 
    tsOffset = *localtime(&epoc);
}

void dateOffset(int year, int month, int day){
  struct tm offsetDate = { 0 };  // set all fields to 0
  offsetDate.tm_year = year - 1900;
  offsetDate.tm_mon = month;
  offsetDate.tm_mday = day;
  offsetDate.tm_hour = ts.tm_hour;
  offsetDate.tm_sec = ts.tm_sec;
  offsetDate.tm_isdst = -1;  // Let system determine if DST was in effect
  time_t offsetTime = mktime(&offsetDate);
  time_t now = mktime(&ts);
  timeOffset((long)difftime(now, offsetTime),true);
}

//return current time year
int timeYear(void){    
    if(offsetEn) return tsOffset.tm_year + 1900;        
    return ts.tm_year + 1900; 
    }

//return current time month
int timeMonth(void){
    if(offsetEn) return tsOffset.tm_mon;
   return ts.tm_mon;
}

//return current time day
int timeDay(void){
    if(offsetEn) return tsOffset.tm_mday;
    return ts.tm_mday;
}

//return current time hour
int timeHour(void){
    if(offsetEn) return tsOffset.tm_hour;
    return ts.tm_hour;
}

//return current time minutes
int timeMinute(void){
    if(offsetEn)  return tsOffset.tm_min;
    return ts.tm_min;
}

//return current time seconds
int timeSecond(void){
    if(offsetEn) return tsOffset.tm_sec;
    else return ts.tm_sec;
}



