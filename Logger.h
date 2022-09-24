#ifndef LOGGER_H
#define LOGGER_H

#define LOG_EMERG     0 /* system is unusable */
#define LOG_ALERT     1 /* action must be taken immediately */
#define LOG_CRIT      2 /* critical conditions */
#define LOG_ERR       3 /* error conditions */
#define LOG_WARNING   4 /* warning conditions */
#define LOG_NOTICE    5 /* normal but significant condition */
#define LOG_INFO      6 /* informational */
#define LOG_DEBUG     7 /* debug-level messages */


// ********************************************************************
// for SYSLOG
#include <WiFiUdp.h>      
#include <Syslog.h>


// ********************************************************************
// for WebSerialSM
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "WebSerialSM.h"

// ********************************************************************
// for NTP client
#include <NTPClient.h>



typedef struct {
  IPAddress serverIP;
  uint16_t  port;
  char      deviceName[30];
  char      appName[30];
} tSyslogParam;

typedef struct {
  uint16_t        port;
  char            path[30];
  RecvMsgHandler  cbMsgHandler;
  void*           cbContext;
} tWebSerialParam;

typedef struct {
  const char*      poolServerName;
  long             timeOffset;
  unsigned long    updateInterval;
} tNTP_Param;


class Logger {
private:

  tWebSerialParam webSerialParam;

  tSyslogParam    syslogParam;
  tNTP_Param      ntpParam;

  WiFiUDP         *udpClient;
  Syslog          *syslog;
  AsyncWebServer  *serverWeb;
  NTPClient       *timeClient;


  char            *buffer;
  uint16_t        bufferSize;
  
  uint32_t        serialSpeed;

  static void cbWebSerialConnect(void *context, bool isConnected);
  static void cbWebSerialMsg(void *context, char *msg);
   
public:
  Logger (uint32_t serialSpeed, uint16_t bufferSize);
  void initSyslog(char *deviceName, char *appName, IPAddress serverIP = IPAddress(192,168,0,7), uint16_t port = 514);
  void initWebSerial(RecvMsgHandler  cbMsgHandler, void* context, char *path="/log", uint16_t port = 80);
  void initSerial(uint32_t serialSpeed);
  void initNTP(const char* poolServerName="europe.pool.ntp.org", long timeOffset=3600, unsigned long updateInterval=60000);
  
  void begin();

  void printf(uint8_t pri, char *fmt, ...);

};

extern Logger Log;

#endif 
