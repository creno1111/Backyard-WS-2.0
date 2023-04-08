#include "input.h"
#include "Arduino.h"
#include "FileSvc.h"


extern int type;                             // animation to run
extern int maxType;                              // max animations
extern RTC_DATA_ATTR bool resetCMD;          // reset wifi credential bool





/*********************
 * Serial CMD Processor
 */
void serialProcess(void){
  String command;
  if(Serial.available()){ command = Serial.readStringUntil('\n');
  // reset wifi credentials and deepsleep/reboot PowerMonitor (deepsleep used to retain reset credentals indicator)
    if(command=="rwifi"){ resetCMD=true; esp_sleep_enable_timer_wakeup(1 * 1000); esp_deep_sleep_start(); }
  // reboot PowerMonitor
    else if(command=="r"){ ESP.restart(); }
  // set brightness
    else if(command[0]=='b' && (command[1]-48 >= 0 && command[1]-48 <= 9)){
      int bright=0; int intNum=1; 
      for(int x=3;x > 0;x--){
        if(command[x]-48 >= 0 && command[x]-48 <= 9){ 
          if(intNum==3) bright+= (command[x]-48) *100; 
          if(intNum==2) bright+= (command[x]-48) *10; 
          if(intNum==1) bright+= (command[x]-48) *1; 
          intNum++; }
      }
      if(bright > 100) bright=100;
      Serial.printf("brightness @ %i%%\n",bright);
      // brightness = map(bright,0,100,0,255);
    } else if(command[0] == 's'){ 

      if(command[1]=='l'){
         Serial.print("List - ");
      //  listCache();
        return;}
      if(command[1]=='s'){
         Serial.print("Save - ");
       // paletteUpdater(0,_paletteUpd);
      //  paletteUpdater(_paletteSave, _paletteUpd);
        return;}
      if(command[1]=='r'){
         Serial.print("Read - ");
       // paletteUpdater(0,_paletteRead);
        return;}

    }
  }
}