#include "Network.h"
#include <ezTime.h>
#include "WebSvc.h"
#include "HTTPClient.h"
#include "sensors.h"
#include "fileSvc.h"
// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _WIFIMGR_LOGLEVEL_    0


#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>
// From v1.1.0
#include <WiFiMulti.h>
WiFiMulti wifiMulti;



extern float WindDir;
extern float WindGust;
extern FS* filesystem;
extern bool test_board;
extern struct settingsWS settings_WS;

#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#define LED_ON      HIGH
#define LED_OFF     LOW
extern RTC_DATA_ATTR bool resetCMD; 
int CPTime=120;
bool netActive=true;
extern int maxType;
bool firstStartup=true;

#include <ESPmDNS.h>

extern int type;

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

// From v1.1.0
#define MIN_AP_PASSWORD_SIZE    8

#define SSID_MAX_LEN            32
//From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN            64

typedef struct
{
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw  [PASS_MAX_LEN];
}  WiFi_Credentials;

typedef struct
{
  String wifi_ssid;
  String wifi_pw;
}  WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS      2

typedef struct
{
  WiFi_Credentials  WiFi_Creds [NUM_WIFI_CREDENTIALS];
} WM_Config;

WM_Config         WM_config;

#define  CONFIG_FILENAME              F("/wifi_cred.dat")

// Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
bool initialConfig = false;
//////

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h>
#define USE_AVAILABLE_PAGES     false

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
//#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP     true
#define USING_AFRICA        false
#define USING_AMERICA       true
#define USING_ANTARCTICA    false
#define USING_ASIA          false
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     false
#define USING_EUROPE        false
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP          false

// New in v1.0.11
#define USING_CORS_FEATURE          true
//////

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
  // Force DHCP to be true
  #if defined(USE_DHCP_IP)
    #undef USE_DHCP_IP
  #endif
  #define USE_DHCP_IP     true
#else
  // You can select DHCP or Static IP here
  #define USE_DHCP_IP     true
  //#define USE_DHCP_IP     false
#endif

#if ( USE_DHCP_IP || ( defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP ) )
// Use DHCP
  IPAddress stationIP   = IPAddress(0, 0, 0, 0);
  IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);
#else
  // Use static IP
  
  #ifdef ESP32
    IPAddress stationIP   = IPAddress(192, 168, 2, 232);
  #else
    IPAddress stationIP   = IPAddress(192, 168, 2, 186);
  #endif
  
  IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);
#endif

#define USE_CONFIGURABLE_DNS      true

IPAddress dns1IP      = gatewayIP;
IPAddress dns2IP      = IPAddress(8, 8, 8, 8);

#include <ESP_WiFiManager.h>              //https://github.com/khoih-prog/ESP_WiFiManager
#include <ESP_WiFiManager_Debug.h>

const char* host = "WSv2";


// Function Prototypes
uint8_t connectMultiWiFi(void);


void heartBeatPrint(void)
{

}

void check_WiFi(void)
{
  if ( (WiFi.status() != WL_CONNECTED) )
  {
    Serial.println("\nWiFi lost. Call connectMultiWiFi in loop");
    connectMultiWiFi();
  }
}  

void check_status(void)
{
  static ulong checkstatus_timeout  = 0;
  static ulong checkwifi_timeout    = 0;

  static ulong current_millis;

#define WIFICHECK_INTERVAL    1000L
#define HEARTBEAT_INTERVAL    10000L

  current_millis = millis();
  
  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0))
  {
    check_WiFi();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}

void loadConfigData(void)
{
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  if (file)
  {
    file.readBytes((char *) &WM_config, sizeof(WM_config));
    file.close();
    LOGERROR(F("OK"));
  }
  else
  {
    LOGERROR(F("failed"));
  }
}

void saveConfigData(void)
{
  File file = FileFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveWiFiCfgFile "));

  if (file)
  {
    file.write((uint8_t*) &WM_config, sizeof(WM_config));
    file.close();
    LOGERROR(F("OK"));
  }
  else
  {
    LOGERROR(F("failed"));
  }
}

uint8_t connectMultiWiFi(void)
{
#if ESP32
  // For ESP32, this better be 0 to shorten the connect time
  #define WIFI_MULTI_1ST_CONNECT_WAITING_MS       0
#else
  // For ESP8266, this better be 2200 to enable connect the 1st time
  #define WIFI_MULTI_1ST_CONNECT_WAITING_MS       2200L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS           100L
  
  uint8_t status;

  LOGERROR(F("ConnectMultiWiFi with :"));
  
  if ( (Router_SSID != "") && (Router_Pass != "") )
  {
    LOGERROR3(F("* Flash-stored Router_SSID = "), Router_SSID, F(", Router_Pass = "), Router_Pass );
  }

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
  {
    // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
    if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
    {
      LOGERROR3(F("* Additional SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
    }
  }
  
  LOGERROR(F("Connecting MultiWifi..."));

  WiFi.mode(WIFI_STA);

#if !USE_DHCP_IP    
  #if USE_CONFIGURABLE_DNS  
    // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
    WiFi.config(stationIP, gatewayIP, netMask, dns1IP, dns2IP);  
  #else
    // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
    WiFi.config(stationIP, gatewayIP, netMask);
  #endif 
#endif

  int i = 0;
  esp_wifi_set_ps(WIFI_PS_NONE);
  status = wifiMulti.run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ( ( i++ < 10 ) && ( status != WL_CONNECTED ) )
  {
    status = wifiMulti.run();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if ( status == WL_CONNECTED )
  {
    LOGERROR1(F("WiFi connected after time: "), i);
    
    LOGERROR3(F("SSID:"), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
    LOGERROR3(F("Channel:"), WiFi.channel(), F(",IP address:"), WiFi.localIP() );
  }
  else
    LOGERROR(F("WiFi not connected"));

  return status;
}

void wifiSetup(void){
  //display network connection indicator
  // networkConnectShow(WIFI_FOUND);

    Serial.print("Starting AutoConnectAP using " + String(FS_Name));
  Serial.println(" on " + String(ARDUINO_BOARD));

  // START FILESYSTEM, FORMAT IF FAILED
  if (!FileFS.begin(true)) {FileFS.format(); FileFS.begin(true);}

  unsigned long startedAt = millis();

  // Use this to personalize DHCP hostname (RFC952 conformed)
  ESP_WiFiManager ESP_wifiManager("WS2");
  ESP_wifiManager.setDebugOutput(false);


  /*******************************************************************
   * wifi credential clear as test. */
  if(resetCMD)  ESP_wifiManager.resetSettings();


  //set custom ip for portal
  ESP_wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  ESP_wifiManager.setMinimumSignalQuality(-1);

  // From v1.0.10 only
  // Set config portal channel, default = 1. Use 0 => random channel from 1-13
  ESP_wifiManager.setConfigPortalChannel(0);

#if !USE_DHCP_IP    
  #if USE_CONFIGURABLE_DNS  
    // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
    ESP_wifiManager.setSTAStaticIPConfig(stationIP, gatewayIP, netMask, dns1IP, dns2IP);  
  #else
    // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
    ESP_wifiManager.setSTAStaticIPConfig(stationIP, gatewayIP, netMask);
  #endif 
#endif

  // New from v1.1.1
#if USING_CORS_FEATURE
  ESP_wifiManager.setCORSHeader("Your Access-Control-Allow-Origin");
#endif
  Router_SSID = ESP_wifiManager.WiFi_SSID();
  Router_Pass = ESP_wifiManager.WiFi_Pass();

  //scan network and make sure saved network exists before reconnecting
  int n;  int *indices;  int **indicesptr = &indices;  bool netFound=false;
  n = ESP_wifiManager.scanWifiNetworks(indicesptr);
  for (int i = 0; i < n; i++)
  {
    if(!strcmp(WiFi.SSID(indices[i]).c_str(),Router_SSID.c_str())) {
      netFound=true;      Serial.print("Previous AP found: "); Serial.println(WiFi.SSID(indices[i]));
      // networkConnectShow(WIFI_FOUND);
    }
  }
  if(!netFound) {Router_SSID="";}

  //make sure config portal only last for 2 mins. Don't block the program!
  ESP_wifiManager.setConfigPortalTimeout(CPTime); 

  // From v1.1.0, Don't permit NULL password, or 0 (whats returned when not set)
  if ( (Router_SSID == "") || Router_SSID == "0")
  {
    Serial.println("WiFi setup missing, AP config started.");
    // networkConnectShow(WIFI_CP);
    initialConfig = true;

    // Starts an access point
    if ( !ESP_wifiManager.startConfigPortal("Clock", "" ))
      Serial.println("Not connected to WiFi, continuing.");
    else{
      Serial.println("WiFi connected.");
    }

    // Stored for later usage, from v1.1.0, but clear first
    memset(&WM_config, 0, sizeof(WM_config));
    
    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      String tempSSID = ESP_wifiManager.getSSID(i);
      String tempPW   = ESP_wifiManager.getPW(i);
  
      if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);  

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    saveConfigData();
  }
  else
  {
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
  }

  startedAt = millis();

  if (!initialConfig)
  {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    loadConfigData();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    if ( WiFi.status() != WL_CONNECTED ) 
    {
      Serial.println("WiFi Starting");
      // networkConnectShow(WIFI_CONNECT);
      uint8_t wifiStat = connectMultiWiFi();
      if(wifiStat==WL_NO_SHIELD) Serial.println("Status No Shield");
      if(wifiStat==WL_NO_SSID_AVAIL) Serial.println("Status SSID not found");
      if(wifiStat==WL_SCAN_COMPLETED) Serial.println("Status Scan Complet?!");
      if(wifiStat==WL_CONNECTED) Serial.println("Status Connected");
      if(wifiStat==WL_CONNECT_FAILED) Serial.println("Status Connect failed");
      if(wifiStat==WL_CONNECTION_LOST) Serial.println("Status Connection lost");
      if(wifiStat==WL_DISCONNECTED) Serial.println("Status Disconnected");
    }
  }

  Serial.print("WiFi completed, ");
  Serial.print((float) (millis() - startedAt) / 1000L);
  Serial.print(" secs, result is ");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("connected. Local IP: ");
    Serial.println(WiFi.localIP());

    //enable modem light sleep mode for battery savings, disable if hooked to line voltage.
    //WiFi.setSleep(true);
    //HTTPClient http;
  }
  else{
    Serial.println(ESP_wifiManager.getStatus(WiFi.status()));
    // networkConnectShow(WIFI_FAIL);
    netActive=false;
   }


 
  Serial.print(F("HTTP server started @ "));
  Serial.println(WiFi.localIP());

  MDNS.begin(host);
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", HTTP_PORT);
  Serial.print(F("MDNS server started @ ")); Serial.print(host); Serial.println(F(".local"));
  Serial.print(F("Open http://"));  Serial.print(host);  Serial.println(F(".local for main page"));
}

//return SPIFFS partition space data
String FSData(void){ 
  char tmp[50];
  sprintf(tmp, "FS: %.2fMB free, out of %.2fMB", (0.0 + (SPIFFS.totalBytes() - SPIFFS.usedBytes())) / 1000000, (0.0 + SPIFFS.totalBytes()) / 1000000);
  return tmp;
}

//wifi rssi return
int getStrength(int points){
    long rssi = 0;
    long averageRSSI = 0;
    
    for (int i=0;i < points;i++){
        rssi += WiFi.RSSI();
        delay(20);
    }

   averageRSSI = rssi/points;
    return averageRSSI;
}


//get lat and lon from zip code html reqeust. this is done because the NWS api does not support zip code requests
//this is done in code vs HTML client to avoid CORS issues
bool getLatLon(long zip, double &lat, double &lon){
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  char nwsURL[512];
  char tmpZip[6];
  sprintf(tmpZip,"%05d",zip);
  sprintf(nwsURL,"%s%s","https://graphical.weather.gov/xml/sample_products/browser_interface/ndfdXMLclient.php?listZipCodeList=",tmpZip);
  http.begin(nwsURL);
  Serial.printf("%s",nwsURL);
  Serial.print("[HTTP] GET Lat / Lon\n");
  int httpCode = http.GET();
  if(httpCode > 0) {
    Serial.printf("code: %d\n",httpCode);
      if(httpCode == HTTP_CODE_OK) {
          sprintf(nwsURL,"%s",strstr(http.getString().c_str(),"List>"));
          Serial.printf("%s\n",nwsURL);
          String workTmp(nwsURL);
          sprintf(nwsURL,"%s", workTmp.substring(workTmp.indexOf('>')+1,workTmp.indexOf(',')));
          lat = atof(nwsURL);
          sprintf(nwsURL,"%s", workTmp.substring(workTmp.indexOf(',')+1,workTmp.indexOf('<')));
          lon = atof(nwsURL);
          if(lat == 0 || lon == 0){
              Serial.printf("Error: Lat / Lon not found\n");
              http.end();
              return false;
          }
      }
  } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      return false;
  }

  http.end();
  return true;
}



