#include "Logger.h"
#include "EspSaveCrashND.h"

Logger  Log(115200, 1024);

//Offset and size parameters can be passed to the constructor
EspSaveCrash SaveCrash(0x0020, 0x0200);

/* Message callback of WebSerial */
void Logger::cbWebSerialConnect(void *context, bool isConnected){
  Logger* plog = static_cast<Logger*>(context);
  Serial.printf("Connection :%d\n", isConnected); 
}

/* Message callback of WebSerial */
void Logger::cbWebSerialMsg(void *context, char *msg){
  Logger* plog = static_cast<Logger*>(context);  
  Log.printf(LOG_NOTICE, "WMSG - Recieved %s [%p]\n",msg, plog->webSerialParam.cbContext);
  if (plog->webSerialParam.cbMsgHandler) plog->webSerialParam.cbMsgHandler(plog->webSerialParam.cbContext, msg); 
}



Logger::Logger(uint32_t serialSpeed, uint16_t bufferSize) {

  this->bufferSize = bufferSize;
  this->serialSpeed = serialSpeed;
  
  Serial.begin(serialSpeed);
  buffer = (char*) malloc(bufferSize);

  syslogParam.serverIP   = IPAddress(0,0,0,0);
  syslogParam.port       = 0;
  strcpy(syslogParam.deviceName,"");
  strcpy(syslogParam.appName,"");

  ntpParam.poolServerName = NULL;
  ntpParam.timeOffset=0;
  ntpParam.updateInterval=0;

  webSerialParam.cbMsgHandler = NULL;
  webSerialParam.cbContext    = NULL;
  webSerialParam.port         = 0;
  strcpy(webSerialParam.path,"");
  
}


void Logger::initSyslog(char *deviceName, char *appName, IPAddress serverIP, uint16_t port ) {
  syslogParam.serverIP   = serverIP;
  syslogParam.port       = port;
  strcpy(syslogParam.deviceName,  deviceName);
  strcpy(syslogParam.appName,     appName);
}

void Logger::initWebSerial(RecvMsgHandler  cbMsgHandler, void* context, char *path, uint16_t port) {
  webSerialParam.cbMsgHandler = cbMsgHandler;
  webSerialParam.cbContext    = context;
  webSerialParam.port         = port;
  strcpy(webSerialParam.path,path);
  WebSerial.initBuffer(4*1024,200);
}

void Logger::initSerial(uint32_t serialSpeed) {
  this->serialSpeed = serialSpeed;
}

void Logger::initNTP(const char* poolServerName, long timeOffset, unsigned long updateInterval) {
  ntpParam.poolServerName = poolServerName;
  ntpParam.timeOffset     = timeOffset;
  ntpParam.updateInterval = updateInterval;
}


void Logger::begin() {
  Serial.begin(serialSpeed);
  udpClient = new WiFiUDP();

  if (ntpParam.poolServerName) {
    timeClient = new NTPClient(*udpClient, ntpParam.poolServerName, ntpParam.timeOffset, ntpParam.updateInterval);
    timeClient->begin();
    timeClient->update();
    EspSaveCrash::_timeOffset = timeClient->getEpochTime() - (int)(millis()/1000);    
  }
  else 
   EspSaveCrash::_timeOffset = 0;

  if (webSerialParam.port) {
    serverWeb = new AsyncWebServer(webSerialParam.port);
    serverWeb->begin();
    WebSerial.setCallback((void*)this, cbWebSerialMsg, cbWebSerialConnect);
    WebSerial.begin(serverWeb, webSerialParam.path, EspSaveCrash::_timeOffset); 
  }


  if (syslogParam.port) {  
    syslog    = new Syslog(*udpClient, syslogParam.serverIP, syslogParam.port, syslogParam.deviceName, syslogParam.appName, LOG_KERN);
  }
  

 }
  
void Logger::printf(uint8_t pri, char *fmt, ...) {

    va_list argp;
    va_start(argp, fmt);
    vsnprintf( buffer, bufferSize, fmt, argp);  
    va_end(argp);
  
    Serial.printf("%s",buffer);
    if ((webSerialParam.port) && (pri<=LOG_INFO)   ) WebSerial.prints(pri, buffer );
    if ((syslogParam.port)    && (pri<=LOG_NOTICE) ) syslog->logf(pri,buffer);
}
