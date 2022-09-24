#ifndef WEB_SERIAL_SM
#define WEB_SERIAL_SM

#include "Arduino.h"
#include "stdlib_noniso.h"
#include "RotatingBuffer.h"
#include <functional>

#if defined(ESP8266)
    #define HARDWARE "ESP8266"
    #include "ESP8266WiFi.h"
    #include "ESPAsyncTCP.h"
    #include "ESPAsyncWebServer.h"
#elif defined(ESP32)
    #define HARDWARE "ESP32"
    #include "WiFi.h"
    #include "AsyncTCP.h"
    #include "ESPAsyncWebServer.h"
#endif

#define MAX_SPRINTF_SIZE  (4*1024)


typedef std::function<void(void *context, char *data)> RecvMsgHandler;
typedef std::function<void(void *context, bool isConnected)> EvtConnectHandler;

// Uncomment to enable WebSerialSM debug mode
// #define WebSerialSM_DEBUG 1

class WebSerialSM {

public:
    WebSerialSM();
    void initBuffer(short size, short nbMsg);

    void begin(AsyncWebServer *server, const char* url = "/Log", uint32_t timeOffset = 0);
    void setCallback(void* context, RecvMsgHandler _recv, EvtConnectHandler _connect);
    void printf(char *fmt, ...);
    void prints(byte prio, char *str);
    bool getDebug() { return _debug; };
    

private:
    bool              _isConnected  = false;  // s if WebSocket connected
    
    AsyncWebServer   *_server       = NULL;
    AsyncWebSocket   *_ws           = NULL;
    RecvMsgHandler    _recvFunc     = NULL;
    EvtConnectHandler _connectFunc  = NULL;
    void*             _context      = NULL;
    bool              _debug        = false;
    bool              _time         = false;
    RotatingBuffer   *_buf          = NULL;
    char              _strBuf[MAX_SPRINTF_SIZE];
    uint32_t          _timeOffset   = NULL;
          
    void pushLastMsg();
    void addMsg(char* msg, bool store=true);
    
};

extern WebSerialSM WebSerial;
#endif
