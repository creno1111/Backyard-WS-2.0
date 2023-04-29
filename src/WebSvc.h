#ifndef WEBSVC_H
#define WEBSVC_H

#include <WebServer.h>  
#define HTTP_PORT     80
#define FileFS        SPIFFS
#define FS_Name       "SPIFFS"

String getContentType(String filename);
void webSetup(void);
String formatBytes(size_t bytes); 
int calcChecksum(uint8_t* address, uint16_t sizeToCalc);
bool handleFileRead(String path); 
void handleFileUpload(void); 
void handleFileDelete(void); 
void handleFileCreate(void); 
void handleFileList(void);
void handleScheduleList(void);
String handleSchedListJson(void);

void webServerHandle(void);
void handleClockArgs(void); 
void webSocketSvc(void);
void wsRemoteCtrl(void);

void servicePWS(void);

#endif //WEBSVC_H