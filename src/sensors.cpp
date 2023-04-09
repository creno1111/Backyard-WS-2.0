#include "Arduino.h"
#include "sensors.h"
#include "timekeeper.h"

#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
#include <Wire.h>
#include "as5600.h"

//NOTE: if missing BME sensor, sensor auto detect will force test mode.
//      Temp, Humidity, and Barometric pressure are simulated.
//      Wind direction will always be North, wind speed will vary because analog pin will float.
//Test mode is used for testing out the webserver and other functions without sensors.

/*adjustment*/  float WindDirectionOffset = 0.0; //offset degrees, dependent on mouting position, Clockwise 1 deg per 1.00
/*adjustment*/  float WindHoldOffset = 0.35; //Max windspeed bleeddown time (lower slower)
/*adjustment*/  float TempOffset = 0.0;//-2.55; //Temperature offset in deg F

AMS_5600 ams5600;

bool test_board = false;

//windspeed interrupt veriables 
int SamplingRate = 50;                //interrupts per second
int IRAM_ATTR endAngle=0;             //measured angle since last read
static IRAM_ATTR int accAngle = 0;    //total accumulated angle
static int calcResult = 0;            
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
  if(millis() > raDly + 50){
    static float headingLR = 0.0;
    int buffers = 100;
    raDly=millis();
    static float angleRead[100];
    static int count =0 ;
    if(count==buffers) count = 0;
    if(test_board)    angleRead[count] =  angleRead[count] = 0;
    else angleRead[count] = convertRawAngleToDegrees(ams5600.getRawAngle()) + WindDirectionOffset;
    if(angleRead[count] > 360) angleRead[count] = angleRead[count] - 360;
    if(angleRead[count] < 0) angleRead[count] = 360 - angleRead[count];
    float readtmp = angleRead[count];
    count++;  
    if(readtmp > headingLR){if(readtmp - headingLR > 90){int adj=0; while(adj<buffers){angleRead[adj]=readtmp;adj++;}}}
    else if(readtmp < headingLR){if(headingLR - readtmp > 90){int adj=0; while(adj<buffers){angleRead[adj]=readtmp;adj++;}}}
    int a=0;while(a<buffers){heading += angleRead[a];a++;}
    heading = heading / buffers;
    headingLR = heading;
  }
  //return finalAngle;
  char indicator[5];
  if ((heading >= 11.25)&&(heading < 101.25));  // NNE to E 
  {
    if ((heading >= 11.25) && (heading < 33.75))    {      sprintf(indicator,"nne");    } //end if NNE
    if ((heading >= 33.75) && (heading < 56.25))    {      sprintf(indicator,"ne");    }  //end if NE
    if ((heading >= 56.25) && (heading < 78.75))    {      sprintf(indicator,"ene");    }  //end if ENE
    if ((heading >= 78.75) && (heading < 101.25))    {      sprintf(indicator,"e");    }  //end if E
  }    //end if NNE to E
  if ((heading >= 101.25) && (heading < 191.25))    // ESE to S
  {
    if ((heading >= 101.25) && (heading < 123.75))    {      sprintf(indicator,"ese");    }  //end if ESE
    if ((heading >= 123.75) && (heading < 146.25))    {      sprintf(indicator,"se");    }  //end if sE
    if ((heading >= 146.25) && (heading < 168.75))    {      sprintf(indicator,"sse");    }  //end if SSE
    if ((heading >= 168.75) && (heading < 191.25))    {      sprintf(indicator,"s");    }   //end if S
  }    //end if ESE to S
  if ((heading < 281.25) && (heading > 191.25))    // SSW to W
  {
    if ((heading >= 191.25) && (heading < 213.75))    {      sprintf(indicator,"ssw");    }  //end if SSW
    if ((heading >= 213.75) && (heading < 236.25))    {      sprintf(indicator,"sw");    }   //end if SW
    if ((heading >= 236.25) && (heading < 258.75))    {      sprintf(indicator,"wsw");    }  //end if WSW
    if ((heading >= 258.75) && (heading < 281.25))    {      sprintf(indicator,"w");    }    //end if W
  }    //end if SSW to W
  if ((heading >= 281.25) || (heading < 11.25))    // WNW to N
  {
    if ((heading >= 281.25) && (heading < 303.75))    {      sprintf(indicator,"wnw");    }    //end if WNW
    if ((heading >= 303.75) && (heading < 326.25))    {      sprintf(indicator,"nw");    }  //end if NW
    if ((heading >= 326.25) && (heading < 348.75))    {      sprintf(indicator,"nnw");    }  //end if NNW
    if ((heading >= 348.75) || (heading < 11.25))    {      sprintf(indicator,"n");    }   //end if N
  }
  return indicator;
}

//return windspeed reading in MPH
float readWindSpeed(void){
  static float historySpeed=0;
  static float calcSpeed = 0;

  // service mph calculations every 1 second
  if(millis() > calcResult + 1000){

    //windspeed angle buffer (30x oversampling)
    static int aAbuffCnt = 0;
    int samples = 5;
    static int accAngleBuff[5]; 
    if(aAbuffCnt == samples) aAbuffCnt =0;
    accAngleBuff[aAbuffCnt] = accAngle;
    accAngle =0;
    aAbuffCnt++;
    int aabuf = 0;
    while(aabuf < samples){
      accAngle += accAngleBuff[aabuf];
      aabuf++;
    }
    accAngle =  accAngle / samples;
    //Calculate distance traveled
    static long startTime=millis();
    float radiusA = 5.00;       // ** radius in inches of ananometer 
    calcSpeed =  ((radiusA * 3.14) * (((calcResult - startTime)/1000)*60)) * ((float)accAngle / 4095.00);
    startTime = calcResult;
    //convert distance inches per min = mph
    calcResult = millis();
    calcSpeed *=60.00;
    calcSpeed =calcSpeed / 5280.00;
    accAngle = 0;

    if(calcSpeed > historySpeed) historySpeed = calcSpeed;
    else if(historySpeed > WindHoldOffset) {historySpeed = historySpeed - WindHoldOffset; calcSpeed = historySpeed; }
  }
  if(calcSpeed < 0.99) calcSpeed = 0.00;
  return calcSpeed; //return mph 
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
        SensorReading = ((float)(readSensor / readings)/100) + TempOffset;
    } else if(ticker % 2 == 1){ readTrigger=true; }
     if(SensorReading < 200) return SensorReading;
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
    if(SensorReading < 200 && SensorReading > 0) return SensorReading;
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
     if(SensorReading < 200) return SensorReading;
     else return 200; 
}

float readHeatIndex(void){
  return EnvironmentCalculations::HeatIndex(readTemp(), readHumidity(), envTempUnit);
}

