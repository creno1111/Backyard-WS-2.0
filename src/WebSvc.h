#ifndef WEBSVC_H
#define WEBSVC_H

#include <WebServer.h>  
#define HTTP_PORT     80
#define FileFS        SPIFFS
#define FS_Name       "SPIFFS"

String getContentType(String filename);
void webSetup(void);
bool handleFileRead(String path); 
void handleFileUpload(void); 
void handleFileDelete(void); 
void handleFileCreate(void); 
void handleFileList(void);
void webServerHandle(void);
void webSocketSvc(void);
void servicePWS(void);
void SvcArchivalData(void);

#endif //WEBSVC_H