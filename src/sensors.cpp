#include "Arduino.h"
#include "sensors.h"
#include "timekeeper.h"

#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <filesvc.h>
#include "as5600.h"
#include "math.h"

//NOTE: if missing BME sensor, sensor auto detect will force test mode.
//      Temp, Humidity, and Barometric pressure are simulated.
//      Wind direction will always be North, wind speed will vary because analog pin will float.
//Test mode is used for testing out the webserver and other functions without sensors.

AMS_5600 ams5600;

float WindDir; // Wind direction in degrees, 0-360, 0 = North, 90 = East, 180 = South, 270 = West
float WindGust; // Wind gust in mph

bool test_board = false;

extern struct settingsWS settings_WS;

//windspeed interrupt veriables 
int SamplingRate = 50;                //interrupts per second
int IRAM_ATTR endAngle=0;             //measured angle since last read
static IRAM_ATTR double accAngle = 0;    //total accumulated angle
static long calcResult = 0;            
static IRAM_ATTR int startAngle =0;   //measured angle from prior read
hw_timer_t * timer = NULL;            //create timer instance to attach to interrupt
//windspeed interrupt
void IRAM_ATTR windACC(){
  if(netSvc) return;                  //return if services conflict with interrupt - see Websvc.cpp (websocket)
  endAngle = analogRead(33);          //read analog output of angle sensor
  if(endAngle > startAngle) {         
    if(endAngle - startAngle >30) accAngle += (endAngle - startAngle);
  }
  startAngle = endAngle;   
} 

//handle battery charge state reporting
#include "BatteryRead.h"
//Battery Monitor
Pangodream_18650_CL BL(34, 1.70, 20); //1.72
int batteryRead(void){   return BL.getBatteryChargeLevel();}


// Assumed environmental values:
float referencePressure = 3341.86;//1018.6;  // hPa local QFF (official meteor-station reading)
float outdoorTemp = 4.7;           // °C  measured local outdoor temp.
float barometerAltitude = 5414.37;  // meters ... map readings + barometer position
float temp(NAN), hum(NAN), pres(NAN);

#define readings 30
float tempReadings[readings];
int   humidityReadings[readings];
float presReadings[readings];

BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76
);

//#ifndef test_board
BME280I2C bme(settings);
//#endif
EnvironmentCalculations::TempUnit envTempUnit = EnvironmentCalculations::TempUnit_Fahrenheit;

float convertRawAngleToDegrees(word newAngle)
{
  /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */
  float retVal = newAngle * 0.087;
  return retVal;
}

bool sensorsInit()
{
  if(Wire.begin())   Serial.println("Wire Init: ok");
  else Serial.println("Wire Init: NO");
  if(!bme.begin())
  {
    Serial.println("\n\n*** Could not fnd BME280 sensor!\n*** Staring in test board mode\n\n");
    test_board = true;
    delay(1000);
  }
  //timer interrupt setup
  timer = timerBegin(0, 80, true);                 //Begin timer with 1 MHz frequency (80MHz/80)
  timerAttachInterrupt(timer, &windACC, true);     //Attach the interrupt to Timer1
  unsigned int timerFactor = 1000000/SamplingRate; //Calculate the time interval between two readings, or more accurately, the number of cycles between two readings
  timerAlarmWrite(timer, timerFactor, true);       //Initialize the timer
  timerAlarmEnable(timer);
  // setOffsets();
  settings_WS = readSettings();
  return true;
}

void sensorsSvc(void){
  static long readDelay = 0;
  if(millis() > readDelay + 1000){
    readDelay = millis();
    if(test_board){    pres = 30.00;     temp = 72.00;     hum = 40;}
    else { BME280::TempUnit tempUnit(BME280::TempUnit_Fahrenheit);
      BME280::PresUnit presUnit(BME280::PresUnit_inHg);
      bme.read(pres, temp, hum, tempUnit, presUnit);
    }
  }
}

String readWindDirection(void){
  static long raDly = 0;
  static float heading=0.0;
  float headingTmp = 0.0;
  if(millis() > raDly + 50){
    static float headingLR = 0.0;
    int buffers =400;
    raDly=millis();
    static float angleRead[400];
    static int count =0 ;
    if(count==buffers) count = 0;
    else angleRead[count] = test_board ? random(0,360) : convertRawAngleToDegrees(ams5600.getRawAngle());
    if(angleRead[count] > 360) angleRead[count] = angleRead[count] - 360;
    if(angleRead[count] < 0) angleRead[count] = 360 + angleRead[count];
    float readtmp = angleRead[count];
    count++;  
    if(readtmp > headingLR){if(readtmp - headingLR > 90){int adj=0; while(adj<buffers){angleRead[adj]=readtmp;adj++;}}}
    else if(readtmp < headingLR){if(headingLR - readtmp > 90){int adj=0; while(adj<buffers){angleRead[adj]=readtmp;adj++;}}}
    int a=0;while(a<buffers){heading += angleRead[a];a++;}
    heading = heading / buffers;
    headingLR = heading;
    headingTmp = heading + (float)settings_WS.WindDir < 0 ? 0 : heading + (float)settings_WS.WindDir;
    if(headingTmp > 360.0) headingTmp = headingTmp - 360;
    else if(headingTmp < 0.0) headingTmp = 360 + headingTmp;
    WindDir = headingTmp;
  }
  //return finalAngle;
  char indicator[5];
  if ((headingTmp >= 11.25)&&(headingTmp < 101.25));  // NNE to E 
  {
    if ((headingTmp >= 11.25) && (headingTmp < 33.75))    {      sprintf(indicator,"nne");    } //end if NNE
    if ((headingTmp >= 33.75) && (headingTmp < 56.25))    {      sprintf(indicator,"ne");    }  //end if NE
    if ((headingTmp >= 56.25) && (headingTmp < 78.75))    {      sprintf(indicator,"ene");    }  //end if ENE
    if ((headingTmp >= 78.75) && (headingTmp < 101.25))    {      sprintf(indicator,"e");    }  //end if E
  }    //end if NNE to E
  if ((headingTmp >= 101.25) && (headingTmp < 191.25))    // ESE to S
  {
    if ((headingTmp >= 101.25) && (headingTmp < 123.75))    {      sprintf(indicator,"ese");    }  //end if ESE
    if ((headingTmp >= 123.75) && (headingTmp < 146.25))    {      sprintf(indicator,"se");    }  //end if sE
    if ((headingTmp >= 146.25) && (headingTmp < 168.75))    {      sprintf(indicator,"sse");    }  //end if SSE
    if ((headingTmp >= 168.75) && (headingTmp < 191.25))    {      sprintf(indicator,"s");    }   //end if S
  }    //end if ESE to S
  if ((headingTmp < 281.25) && (headingTmp > 191.25))    // SSW to W
  {
    if ((headingTmp >= 191.25) && (headingTmp < 213.75))    {      sprintf(indicator,"ssw");    }  //end if SSW
    if ((headingTmp >= 213.75) && (headingTmp < 236.25))    {      sprintf(indicator,"sw");    }   //end if SW
    if ((headingTmp >= 236.25) && (headingTmp < 258.75))    {      sprintf(indicator,"wsw");    }  //end if WSW
    if ((headingTmp >= 258.75) && (headingTmp < 281.25))    {      sprintf(indicator,"w");    }    //end if W
  }    //end if SSW to W
  if ((headingTmp >= 281.25) || (headingTmp < 11.25))    // WNW to N
  {
    if ((headingTmp >= 281.25) && (headingTmp < 303.75))    {      sprintf(indicator,"wnw");    }    //end if WNW
    if ((headingTmp >= 303.75) && (headingTmp < 326.25))    {      sprintf(indicator,"nw");    }  //end if NW
    if ((headingTmp >= 326.25) && (headingTmp < 348.75))    {      sprintf(indicator,"nnw");    }  //end if NNW
    if ((headingTmp >= 348.75) || (headingTmp < 11.25))    {      sprintf(indicator,"n");    }   //end if N
  }
  return indicator;
}


//return windspeed reading in MPH
float readWindSpeed(void){
  static float calcSpeed = 0;
  static long startTime=millis();
  int WindGustHold1 = 3600; //seconds to hold first Gust
  int windGustHold2 = 2700; //seconds before second level Gust starts recording
  // service mph calculations every 1 second
  if(millis() > calcResult + 1000){
    float offsetTime = (millis() - calcResult)/1000; //calculate time from last reading
    if(accAngle < offsetTime*600) accAngle = 0; //remove analog noise on no wind conditions

    // //windspeed angle buffer (5x oversampling) 
    static int aAbuffCnt = 0;
    int samples = 5;
    static double accAngleBuff[5]; 
    if(aAbuffCnt == samples) aAbuffCnt = 0;
    accAngleBuff[aAbuffCnt] = accAngle / offsetTime;
    accAngle =0;
    aAbuffCnt++;
    int aabuf = 0;
    while(aabuf < samples){
      accAngle += accAngleBuff[aabuf];
      aabuf++;
    }
    accAngle =  accAngle / samples;
    
    //Calculate distance traveled (mph)
    calcSpeed = pow((accAngle / 4095.00) / 0.1, 0.55) * 2.23694;
    calcResult = millis();
    accAngle=0;

    //don't allow the first few readings to be used for wind gust
    static bool firstRun = true;
    if(firstRun){
      static int ignoreGusts = 0;
      ignoreGusts++;
      if(ignoreGusts = 5) firstRun = false;
      calcSpeed = 0;
    }  

    // Calculate the wind gust, get top speed over 5 minutes
    float windSpeed = calcSpeed; // The current wind speed
    static float maxWindGust = 0; // The maximum wind gust
    static float SecondHighestWindGust = 0; // The second highest wind gust
    static long maxHold=0;
    if (windSpeed > maxWindGust)
    {
      maxHold = millis();  
      maxWindGust = windSpeed;
      SecondHighestWindGust = 0;
    }
    if(millis() > maxHold + (windGustHold2 * 1000)){
      if (windSpeed > SecondHighestWindGust) SecondHighestWindGust = windSpeed;
    }
    if(millis() > maxHold + (WindGustHold1 * 1000)){
      maxWindGust = SecondHighestWindGust; maxHold = millis();
      SecondHighestWindGust = 0;
    }
    WindGust = maxWindGust;
    
    // running average of windspeed, with a smooting window
    int WINDOW_SIZE = 25; //must be lower than Samples
    int Samples = 60;
    static int x=0;
    static float windspeed[60 /*Match Samples*/];
    windspeed[x] = calcSpeed;
    x++;
    if(x==Samples) x=0;
    float smoothed[60/*Match Samples*/];
    int i, j;
    float sum = 0.0;
    for (i = 0; i < Samples; i++) {
        if (i < WINDOW_SIZE - 1) {
            smoothed[i] = windspeed[i];
            calcSpeed = smoothed[i];
        } else {
            sum = 0.0;
            for (j = 0; j < WINDOW_SIZE; j++) {
                sum += windspeed[i - j];
            }
            smoothed[i] = sum / WINDOW_SIZE;
            calcSpeed = smoothed[i];
        }
    }
  }
  if(calcSpeed < 0.99) calcSpeed = 0.00;
  return calcSpeed + settings_WS.WindOffset < 0 ? 0 : calcSpeed + settings_WS.WindOffset;
}

//return temp readings averaged out over 30 readings. ???.?? f
float readTemp(void){
    int ticker = timeSecond();
    long readSensor=0;
    static float SensorReading;
    static bool readTrigger = true;
    static bool firstReading = true;  if(firstReading){ firstReading = false; for(int a=0;a<readings;a++){ tempReadings[a] = temp;  }  }   //init buffer
    if(ticker % 2 == 0 && readTrigger){  if(ticker!=0) ticker = ticker / 2;   readTrigger = false;
        tempReadings[ticker] = temp;
        for(int a=0; a<readings;a++){ readSensor += tempReadings[a] * 100;  }
        SensorReading = ((float)(readSensor / readings)/100);
    } else if(ticker % 2 == 1){ readTrigger=true; }
     if(SensorReading < 200) return SensorReading + settings_WS.TempOffset < 0 ? 0 : SensorReading + settings_WS.TempOffset;
     else return 200; 
}

//return humidity readings averaged out over 30 readings. ?? %
int readHumidity(void){  
    int ticker = timeSecond();
    long readSensor=0;
    static int SensorReading=0;
    static bool readTrigger = true;
    static bool firstReading = true;  if(firstReading){ firstReading = false; for(int a=0;a<readings;a++){ humidityReadings[a] = hum;  }  }   //init buffer
    if(ticker % 2 == 0 && readTrigger){  if(ticker!=0) ticker = ticker / 2;   readTrigger = false;
         humidityReadings[ticker] = hum;
        for(int a=0; a<readings;a++){ readSensor += humidityReadings[a]; }
        SensorReading = readSensor / readings;
    } else if(ticker % 2 == 1){ readTrigger=true; }
    if(SensorReading < 200 && SensorReading > 0) return SensorReading + settings_WS.HumidityOffset < 0 ? 0 : SensorReading + settings_WS.HumidityOffset;
    else return 200; 
}

//return pressure readings averaged out over 30 readings. ??.??
float readPressure(void){
/* adjustment */ float pressureAdj = 0.9;
    int ticker = timeSecond();
    long readSensor=0;
    static float SensorReading;
    static bool readTrigger = true;
    static bool firstReading = true;  if(firstReading){ firstReading = false; for(int a=0;a<readings;a++){ presReadings[a] = pres + pressureAdj;  }  }   //init buffer
    if(ticker % 2 == 0 && readTrigger){  if(ticker!=0) ticker = ticker / 2;   readTrigger = false;
        presReadings[ticker] = pres + pressureAdj;
        for(int a=0; a<readings;a++){ readSensor += presReadings[a] * 100;  }
        SensorReading = (float)(readSensor / readings)/100;
    } else if(ticker % 2 == 1){ readTrigger=true; }
     if(SensorReading < 200) return SensorReading + settings_WS.BaroOffset < 0 ? 0 : SensorReading + settings_WS.BaroOffset;
     else return 200; 
}

float readHeatIndex(void){
  return EnvironmentCalculations::HeatIndex(readTemp(), readHumidity(), envTempUnit);
}

float readDewPoint(void){
  return EnvironmentCalculations::DewPoint(readTemp(), readHumidity(), envTempUnit);
}
