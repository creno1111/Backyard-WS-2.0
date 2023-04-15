#include "Arduino.h"
#include "WebSvc.h"
#include "Network.h"
#include "FileSvc.h"
#include "input.h"
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <update.h>
#include "string.h"
#include "timekeeper.h"
#include "sensors.h"
#include "arduinojson.h"


extern float minuteGraphTemp[60];
extern int minuteGraphHumidity[60];
extern float minuteGraphBarometric[60];
extern float minuteGraphWindspeed[60];
extern float dayGraphTemp[96];
extern int dayGraphHumidity[96];
extern float dayGraphBarometric[96];
extern float dayGraphWindspeed[96];
extern struct settingsWS settings_WS;

static bool zipReturn = true;

//extern int minute;

WebServer server(HTTP_PORT);
extern FS* filesystem;
WebSocketsServer webSocket = WebSocketsServer(1883);

//holds the current upload
File fsUploadFile;

using namespace std;

extern bool netActive;
extern bool digitOverlay;
int value1=0;
int value2=0;
bool value1En = false;
bool value2En = false;
extern bool rstAH;

//bubble sort for Web FileList
void fileSort(JsonArray& arr);

//private define 
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


/***********************************************************************************************/
//setup
void webSetup(void){
    //SERVER INIT
  
  //main index file, Loads from SPIFFS. 
  //If missing, loads embeded uploader
    server.on("/", HTTP_GET, []()
  {
    if (!handleFileRead("/index.htm")) 
      {
        server.send(404, "text/html", "<html><head><meta http-equiv='refresh' content='1; url=/uploader'>");   
      }
      zipReturn = true;
  });

  // FLASH EMBEDED UPLOADER 
  // Handles OTA updates, Flash & Partition.
  // Handles SPIFFS file uploads, downloads & deletes
  server.on("/uploader", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", 
      "<meta http-equiv='Content-type' name='viewport' content='text/html, charset=utf-8, width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1'/>"
      "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
      "<center><p>Select your upload type</p>"
      "<button class=btn  onclick=\"location.href='/';\" style='border:2px solid #400000;'><- BACK</button>"
      "<button class=btn  onclick=\"onPressFI()\" style='border:2px solid #800000;'>FILE</button>"
      "<button class=btn  onclick=\"onPressFW()\" style='border:2px solid #800000;'>FIRMWARE</button>"
      "<button class=btn  onclick='onPressPA()' style='border:2px solid #800000;'>PARTITION</button></center>"
      "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
      "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none accept='*.*'>"
      "<label id='file-input' for='file'><center><span id='fileMsg'>[ Project File ]</span></center></label>"
      "<input type='submit' class=btn value='UPLOAD'><br><br>"
      "<div id='prg'><span style='color:grey'>Select file, Press UPLOAD</span></div>"
      "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
      "<div id='flistbox'><h3>FS FILE LIST</h3><output id='filelist'></output>" + FSData() + "</div>"
      "<iframe id='fdownload' style='display:none;'></iframe><script>"
      "var updateType = '/updfile';function onPressFW(){	updateType = '/updfirmware';document.getElementById('fileMsg').innerHTML = '[ Firmware File ]';"
      "document.getElementById('file').accept = '.bin';}"
      "function onPressPA(){	updateType = '/updpartition';document.getElementById('fileMsg').innerHTML = '[ Partition File ]';"
      "document.getElementById('file').accept = '.bin';}"
      "function onPressFI(){updateType = '/updfile';document.getElementById('fileMsg').innerHTML = '[ Project File ]';"
      "document.getElementById('file').accept = '*.*';}"
      "function sub(obj){var fileName = obj.value.split('\\\\');document.getElementById('file-input').innerHTML = '   <center>'+ fileName[fileName.length-1] +'</center>';};"
      "$('form').submit(function(e){e.preventDefault();var form = $('#upload_form')[0];var data = new FormData(form);$.ajax({url: updateType,"
      "type: 'POST',data: data,contentType:false,processData:false,xhr: function(){var xhr = new window.XMLHttpRequest();xhr.upload.addEventListener('progress', function(evt) {"
      "if (evt.lengthComputable){var per = evt.loaded / evt.total;$('#prg').html('' + Math.round(per*100) + '%');$('#bar').css('width',Math.round(per*100) + '%');"
      "}}, false);return xhr;},success:function(d, s){if(updateType == '/updfirmware' || updateType == '/updpartition'){$('#prg').html('Please wait, restarting');"
      "setTimeout(function (){window.location.href = '/';}, 8000);}else{$('#prg').html('Upload completed');$('#bar').css('width','0%');setTimeout(function () {"
      "window.location.href = '/uploader';}, 300);}},error: function (a, b, c) {}});});"
      "var req = new XMLHttpRequest();req.open('GET', '/list?dir=/', false);req.send(null);"
      "var jdata = '{\"filelist\":' + req.responseText + '}';console.log(jdata);var jsonData = JSON.parse(jdata);"
      "var list = document.getElementById('filelist'); if (list != null) {for(var i = jsonData.filelist.length; i --> 0;) {"
      "var z = document.createElement('p');z.innerHTML = \"<button class=btn onclick=\\\"fdownload('/\" + jsonData.filelist[i].name + \"')\\\">&#8595; </button><button class=btn onclick=\\\"fdelete('/\" + jsonData.filelist[i].name + \"')\\\">X </button><span id='flist'> \" + jsonData.filelist[i].name + jsonData.filelist[i].size + \"</span>\";"
      "list.appendChild(z);}} else console.log('hmm');"
      "function fdelete(a){var req = new XMLHttpRequest();req.open('DELETE', '/edit?itemid=' + a, false);req.send(null);setTimeout(function () {window.location.href = '/uploader';}, 300);}"
      "function fdownload(a){document.getElementById('fdownload').src = a + \"?download=true\";}"
      "</script><style>#file-input,input{	width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}input{background:#000000;border:0;"
      "padding:0 15px}body{background:#000000;font-family:sans-serif;font-size:14px;color:#ff0000;font-weight: bold;text-shadow: 0px 0px 6px #000000}"
      "#file-input{background:#202040;padding:0;border:1px solid #000000;line-height:44px;text-align:left;display:block;cursor:pointer}#bar,#prgbar{background-color:#202040;"
      "border-radius:10px;}#bar{background-color:#ff0000;width:0%;height:10px;color:#ff0000;font-weight: bold;text-shadow: 0px 0px 6px #ffffff"
      "}form{background:#000000;max-width:258px;margin:10px auto;padding:10px;border-radius:5px;text-align:center; border:solid;}table{background:#000000;"
      "max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}p{background:#000000;max-width:258px;border-radius:5px;"
      "}.btn{background:#202040;color:#ff0000;border:1px solid #000000;text-shadow: 0px 0px 6px #000000;font-weight: bold;border-radius:5px;"
      "cursor:pointer}#flist {line-height: 5px}#flistbox{margin:auto;width:256px;border:4px solid #ff0000;border-radius:5px;padding:9px;}.btn:hover,#file-input:hover{background:#101010;}</style>"
    );
    
  });
  /*handling uploading firmware file */
  server.on("/updfirmware", HTTP_POST, []() {
    Serial.println("firmware, here");
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(200);    yield();    delay(200);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if(filename.length() < 1) return;
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)) { //start with max available size
        Update.printError(Serial);
      }
      // updLedEnable();updLed();neoPixelShow();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
      // updLedEnable();updLed();neoPixelShow();
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      // resetLeds();
      } else {
        Update.printError(Serial);
      }
    }
  });
  /*handling uploading partition file */
  server.on("/updpartition", HTTP_POST, []() {
    Serial.println("partition, here");
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(200);    yield();    delay(200);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if(filename.length() < 1) return;
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN,U_SPIFFS)) { //start with max available size
        Update.printError(Serial);
      }
      // updLedEnable();updLed();neoPixelShow();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
      // updLedEnable();updLed();neoPixelShow();
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      // resetLeds();
      } else {
        Update.printError(Serial);
      }
    }
  });
/*handling uploading FS file */
  server.on("/updfile", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  }, []() {
  HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) 
    {
      String filename = upload.filename;
      if(filename.length() < 1) return;
      if (!filename.startsWith("/"))       {        filename = "/" + filename;      }
      Serial.print(F("handleFileUpload Name: ")); Serial.println(filename);
      fsUploadFile = filesystem->open(filename, "w");
      filename.clear();
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) 
    { if (fsUploadFile){fsUploadFile.write(upload.buf, upload.currentSize);} } 
    else if (upload.status == UPLOAD_FILE_END) 
    {if (fsUploadFile){fsUploadFile.close();}Serial.print(F("handleFileUpload Size: ")); Serial.println(upload.totalSize); }
  });  

  /**************************
   * file list, create, delete and upload handler calls
   */
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() 
  {server.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined
  //use it to load files from SPIFFS
  //if not found - loads main page
  server.onNotFound([]() 
  {if (!handleFileRead(server.uri())) {server.send(404, "text/html", "<html><head><meta http-equiv='refresh' content='1; url=/'>");} });

  //handle favicon.ico web site tab icon
  server.on("/favicon.ico", HTTP_GET, []()
  {if (!handleFileRead("/favicon.ico")) {server.send(404, "text/plain", "FileNotFound");}});

  //start web server
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

/***********************************************************************************************/
//get content type
String getContentType(String filename) 
{
  if (server.hasArg("download")) {return "application/octet-stream";} 
  else if (filename.endsWith(".htm")) {return "text/html";} 
  else if (filename.endsWith(".html")) {return "text/html";} 
  else if (filename.endsWith(".css")) {return "text/css";} 
  else if (filename.endsWith(".js")) {return "application/javascript";} 
  else if (filename.endsWith(".png")) {return "image/png";} 
  else if (filename.endsWith(".gif")) {return "image/gif";} 
  else if (filename.endsWith(".jpg")) {return "image/jpeg";} 
  else if (filename.endsWith(".ico")) {return "image/x-icon";} 
  else if (filename.endsWith(".xml")) {return "text/xml";} 
  else if (filename.endsWith(".pdf")) {return "application/x-pdf";} 
  else if (filename.endsWith(".zip")) {return "application/x-zip";} 
  else if (filename.endsWith(".gz")) {return "application/x-gzip";}
  return "text/plain";
}
/***********************************************************************************************/
//handle file read
bool handleFileRead(String path) 
{
  Serial.println("Serving: " + path);
  if (path.endsWith("/")){
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (filesystem->exists(pathWithGz) || filesystem->exists(path)){
    if (filesystem->exists(pathWithGz)){
      path += ".gz";
    } 
    File file = filesystem->open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
/***********************************************************************************************/
//handle file upload
void handleFileUpload() 
{
  if (server.uri() != "/edit"){
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if (!filename.startsWith("/")){
      filename = "/" + filename;
    }
    Serial.print(F("handleFileUpload Name: ")); Serial.println(filename);
    fsUploadFile = filesystem->open(filename, "w");
    filename.clear();
  } 
  else if (upload.status == UPLOAD_FILE_WRITE){
    if (fsUploadFile){
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END){
    if (fsUploadFile){
      fsUploadFile.close();
    }
    Serial.print(F("handleFileUpload Size: ")); Serial.println(upload.totalSize);
  }
}
/***********************************************************************************************/
//handle file delete
void handleFileDelete() 
{
  if (server.args() == 0){
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if (path == "/"){
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (!filesystem->exists(path)){
    return server.send(404, "text/plain", "FileNotFound");
  }
  filesystem->remove(path);
  server.send(200, "text/plain", "");
  path.clear();
}
/***********************************************************************************************/
//handle file create
void handleFileCreate() 
{
  if (server.args() == 0){
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if (path == "/"){
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (filesystem->exists(path)){
    return server.send(500, "text/plain", "FILE EXISTS");
  }
  File file = filesystem->open(path, "w");
  if (file){
    file.close();
  } 
  else{
    return server.send(500, "text/plain", "CREATE FAILED");
  }
  server.send(200, "text/plain", "");
  path.clear();
}
/***********************************************************************************************/
//handle file list
void handleFileList() 
{
  if (!server.hasArg("dir")){
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }
  String path = server.arg("dir");
  File root = FileFS.open(path);
  path = String();
  String output = "[";
  if (root.isDirectory()){
    File file = root.openNextFile();
    while (file) 
    {
      if (output != "["){
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.name()).substring(0);
      output += "\",\"size\":\"";
      size_t size = file.size();
      char buffer[20];
      if(size > 1024) sprintf(buffer,"  [%i KB] ", (size/1024)+1);
      else sprintf(buffer,"  [%i B] ", size);
      output += String(buffer).substring(0);
      output += "\"}";
      file = root.openNextFile();
    }
  }
  output += "]";
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, output);
  JsonArray data = doc.as<JsonArray>();
  fileSort(data);
  String finalOutput;
  serializeJson(data, finalOutput);
  server.send(200, "text/json", finalOutput);
}

/***********************************************************************************************/
//webserver handle
void webServerHandle(void){
  if(!netActive) return;
  server.handleClient();
  check_status();
} 

char msg_buf[10000];
char msg_buf_tmp[10000];
/**********************************************************************************************/
//websocket events
void dataSeperate(bool clr){  if(clr) msg_buf_tmp[0] = 0x00;  else sprintf(msg_buf_tmp,"%s%s~",msg_buf_tmp,msg_buf);  }
void webSocketEvent(uint8_t num, WStype_t type_ws, uint8_t * payload, size_t length) {
  static bool zipRequest=false;
  String buff = (char *)payload;
  static int graph = 60;
    switch(type_ws) {
      case WStype_DISCONNECTED:
         // Serial.printf("[%u] Disconnected!\n", num);
          break;
      case WStype_CONNECTED:
          {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("WebSocket Client %u Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");             
          }
          break;
      case WStype_TEXT:
        //PRINT RAW MESSAGE TO SERIAL CONSOLE
      //  Serial.printf("[%u] Received text: %s\n", num, payload);
      //  Serial.println(buff.substring(0,buff.indexOf(':')).c_str());
        //HEARTBEAT  RESPONCE
        if  ( strcmp((char *)payload, "HeartBeat") == 0 ){
          dataSeperate(true);
           sprintf(msg_buf, "hb:");
           dataSeperate(false);
         
          //SEND CURRENT TEMP
          sprintf(msg_buf,"Mt:%0.1f",readTemp());
          // Serial.println(msg_buf);          
          dataSeperate(false);

          //SEND CURRENT HUMIDITY
          sprintf(msg_buf,"Mh:%i",readHumidity());
          // Serial.println(msg_buf);          
          dataSeperate(false);

          //SEND CURRENT PRESSURE
          sprintf(msg_buf,"Mp:%.02f",readPressure());
          // Serial.println(msg_buf);    * 
          dataSeperate(false);

           //SEND CURRENT BATTERY CHARGE STATE
           netSvc=true;
          sprintf(msg_buf,"Mb:%i",batteryRead());
           netSvc=false;
          // Serial.println(msg_buf);
          dataSeperate(false);

           //SEND CURRENT WIFI RSSI
          // netSvc=true;
          sprintf(msg_buf,"Mr:%i",abs(getStrength(10)));
          // netSvc=false;
          // Serial.println(msg_buf);
          dataSeperate(false);

          //SEND TIME/DATE
          sprintf(msg_buf,"TD:%i:%02i%s",timeHour() > 12 ? timeHour() -12 : (timeHour() == 0 ? 12 : timeHour()), timeMinute(), timeHour() > 11 ? "pm": "am");
          // Serial.println(msg_buf);          
          dataSeperate(false);

          //SEND WIND DIRECTION
          sprintf(msg_buf,"Md:%s%s",readWindDirection(),"_dir");
         // Serial.println(msg_buf);          
          dataSeperate(false);
          //SEND WIND SPEED
          sprintf(msg_buf,"Mw:%0.2f",readWindSpeed());
         // Serial.println(msg_buf);          
          dataSeperate(false);
          sprintf(msg_buf,"Mi:%0.1f",readHeatIndex());
         // Serial.println(msg_buf);          
          dataSeperate(false);
          //readHeatIndex()
          sprintf(msg_buf,"Gt:%i",graph);
         // Serial.println(msg_buf);          
          dataSeperate(false);
          //readHeatIndex()
         //send lat and lon for NWS data
          //double lat;          double lon;
         // if(zipRequest){
            zipRequest=false;
            //openSettings(lat,lon);
            // settingsWS settings = readSettings();
            /*settingsWS*/ readSettings();
            sprintf(msg_buf,"Ll:%.04f,%.04f",readSettings().lat,readSettings().lon);
            dataSeperate(false);
         // }
          

          //SEND 60M GRAPH DATAPOINTS
          if(graph == 60){
            int dataCnt =0; int tmpMinute=timeMinute()+1;
            if(tmpMinute == 60) tmpMinute =0;
            sprintf(msg_buf, "Tm:");
            while(dataCnt!=60){
              if(tmpMinute > timeMinute()){ timeOffset(3600,1); }
              sprintf(msg_buf, "%s%i,%i,%i,%i,%i,%.01f,%i,%.02f,%.01f,", msg_buf,timeYear(), timeMonth(), timeDay(), timeHour() , tmpMinute, minuteGraphTemp[tmpMinute], minuteGraphHumidity[tmpMinute], minuteGraphBarometric[tmpMinute], minuteGraphWindspeed[tmpMinute]);       
              timeOffset(0,0);
              dataCnt++;
              tmpMinute++;
              if(tmpMinute==60) tmpMinute=0;
            } 
            sprintf(msg_buf, "%s%i,%i,%i,%i,%i,%.01f,%i,%.02f,%.01f", msg_buf,timeYear(), timeMonth(), timeDay(), timeHour() , tmpMinute, minuteGraphTemp[tmpMinute], minuteGraphHumidity[tmpMinute], minuteGraphBarometric[tmpMinute], minuteGraphWindspeed[tmpMinute]); 
            dataSeperate(false);
          }

          // //SEND 24H GRAPH DATAPOINTS
          if(graph == 24){
            int dataCnt =0; int tmpMinute=(timeHour() * 4) + (timeMinute() / 15)+1;
            if(tmpMinute==96) tmpMinute=0;
             sprintf(msg_buf, "Tm:");
             while(dataCnt!=96){
               if(tmpMinute > (timeHour() * 4) + (timeMinute() / 15)){ timeOffset(86400,1); }
               sprintf(msg_buf, "%s%i,%i,%i,%i,%i,%.01f,%i,%.02f,%.01f,", msg_buf,timeYear(), timeMonth(), timeDay(), tmpMinute / 4 , (tmpMinute % 4) * 15 , dayGraphTemp[tmpMinute], dayGraphHumidity[tmpMinute], dayGraphBarometric[tmpMinute], dayGraphWindspeed[tmpMinute]);           
               timeOffset(0,0);
               dataCnt++;
               tmpMinute++;
               if(tmpMinute==96) tmpMinute=0;
             } 
             sprintf(msg_buf, "%s%i,%i,%i,%i,%i,%.01f,%i,%.02f,%.01f", msg_buf,timeYear(), timeMonth(), timeDay(), tmpMinute / 4 , (tmpMinute % 4) * 15 , dayGraphTemp[tmpMinute], dayGraphHumidity[tmpMinute], dayGraphBarometric[tmpMinute], dayGraphWindspeed[tmpMinute]);  
             dataSeperate(false);
          }
          //send zip code, once on initial request
          if(zipReturn){
            zipReturn=false;
            sprintf(msg_buf,"ZC:%05d", settings_WS.zip);
            dataSeperate(false);
          }
          //final line - send sensor data
          //netSvc=true;
          webSocket.sendTXT(num,msg_buf_tmp);
          //netSvc=false;



        //SWITCH TO 60MIN GRAPH
        } else if(  ( strcmp((char *)payload, "60mGraphON") == 0 ) ){
          graph = 60;
        //SWITCH TO 24HOUR GRAPH  
        } else if(  ( strcmp((char *)payload, "24hGraphON") == 0 ) ){
          graph = 24;
        
        //Zip update
        } else if(  ( strstr((char *)payload, "zip:") != NULL ) ){
          zipRequest=true;
          Serial.println("zip update");
          char strTmp[20];
          String workTmp((char *)payload);
          sprintf(strTmp,"%s", workTmp.substring(workTmp.indexOf(':')+1,workTmp.indexOf('\0')));
          Serial.printf("%s\n",strTmp);
          double lat,lon;
          lat=settings_WS.lat;
          lon=settings_WS.lon;
          if(getLatLon(atoi(strTmp),lat,lon)){
            settings_WS.zip=atoi(strTmp); 
            settings_WS.lat=lat;
            settings_WS.lon=lon;
          }else {     zipReturn = true;     }
          writeSettings();
        // reset system
        } else if(  ( strcmp((char *)payload, "resetWS") == 0 ) ){
          webSocket.disconnect(); delay(500); yield(); delay(200); ESP.restart();


      // Message not recognized
        } else {
          Serial.printf("%s",(char *)payload);
          Serial.println("UNKNOWN WEBSOCKET CMD RECEIVED");
        }
        break;  
      case WStype_BIN:
      case WStype_ERROR:			
      case WStype_FRAGMENT_TEXT_START:
      case WStype_FRAGMENT_BIN_START:
      case WStype_FRAGMENT:
      case WStype_FRAGMENT_FIN:
      case WStype_PING:
      case WStype_PONG:
			break;
    }
}

void webSocketSvc(void){ 
  webSocket.loop();
}

// FS file list sorting helper
void fileSort(JsonArray& fileList) {
  int n = fileList.size();
  bool swapped = true;
  while (swapped) {
    swapped = false;
    for (int i = 1; i < n; i++) {
      String current = fileList[i]["name"].as<String>();
      String previous = fileList[i - 1]["name"].as<String>();
      current.toLowerCase();
      previous.toLowerCase();
      if (current > previous) {
        StaticJsonDocument<400> doc;
        JsonObject tmpJsonObj = doc.to<JsonObject>();
        tmpJsonObj["type"]= fileList[i]["type"];
        tmpJsonObj["name"]= fileList[i]["name"];
        tmpJsonObj["size"]= fileList[i]["size"];
        fileList[i]["type"] = fileList[i - 1]["type"];
        fileList[i]["name"] = fileList[i - 1]["name"];
        fileList[i]["size"] = fileList[i - 1]["size"];
        fileList[i - 1]["type"] = tmpJsonObj["type"];
        fileList[i - 1]["name"] = tmpJsonObj["name"];
        fileList[i - 1]["size"] = tmpJsonObj["size"];
        tmpJsonObj.clear();
        swapped = true;
      }
    }
    n--;
  }
}