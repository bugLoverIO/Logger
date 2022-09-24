#include "WebSerialSM.h"
#include "WebSerialSM_webpage.h"
#include "ESPsaveCrashND.h"
#include "RotatingBuffer.h"
#include <time.h>

#define LOG_EMERG     0 /* system is unusable */
#define LOG_ALERT     1 /* action must be taken immediately */
#define LOG_CRIT      2 /* critical conditions */
#define LOG_ERR       3 /* error conditions */
#define LOG_WARNING   4 /* warning conditions */
#define LOG_NOTICE    5 /* normal but significant condition */
#define LOG_INFO      6 /* informational */
#define LOG_DEBUG     7 /* debug-level messages */

extern EspSaveCrash SaveCrash;

WebSerialSM::WebSerialSM() {
  _server       = NULL;
  _ws           = NULL;
  _recvFunc     = NULL;
  _connectFunc  = NULL;
  _isConnected  = false;
  _debug        = false;
  _time         = true;
  _buf          = NULL;  
  _timeOffset   = NULL;
}

void WebSerialSM::initBuffer(short size, short nbMsg){
  _buf = new  RotatingBuffer(size, nbMsg);  
}


void WebSerialSM::begin(AsyncWebServer *server, const char* url, uint32_t timeOffset){
  
    _server = server;
    _ws = new AsyncWebSocket("/webserialws");
    _timeOffset = timeOffset;    

    _server->on(url, HTTP_GET, [](AsyncWebServerRequest *request){
        // Send Webpage
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", WEBSERIAL_HTML, WEBSERIAL_HTML_SIZE);
        response->addHeader("Content-Encoding","gzip");
        request->send(response);   
        
    });

    _ws->onEvent([&](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) -> void {
        if(type == WS_EVT_CONNECT){
            _isConnected = true; 
       
            SaveCrash.print(_strBuf,MAX_SPRINTF_SIZE - 1);
            _ws->textAll(_strBuf);

            if ( (_buf) && !(_buf->isEmpty()) ) {
              _ws->textAll("Last saved messages\n");            
              pushLastMsg();
            } else _ws->textAll("No message saved\n");             

            if (_debug) _ws->textAll("#DebugON");            
            if (_time) _ws->textAll("#TimeON");
            
            if (_connectFunc) _connectFunc(_context, true); 


            
        } else if(type == WS_EVT_DISCONNECT){
            
            if (_connectFunc) _connectFunc(_context, false); 
            _isConnected = false;
            
        } else if(type == WS_EVT_DATA){
            
            char *msg = (char*)data;
            msg[len] = 0;
            
            if (_ws->availableForWriteAll()) pushLastMsg();
          
            if (strcmp(msg,"#CleanCrash#") == 0) {
              SaveCrash.clear();
              _ws->textAll("Crash logged clearer\n");
            } else if (strcmp(msg,"#Reset#") == 0) {
              _ws->textAll("Reset on going ...\n");
              ESP.reset();
            } else if (strcmp(msg,"#DebugON#") == 0) {
              _debug = true;
              _ws->textAll("Debug = ON\n");
            } else if (strcmp(msg,"#DebugOFF#") == 0) {
              _debug = false;
              _ws->textAll("Debug = OFF \n");
            } else if (strcmp(msg,"#TimeON#") == 0) {
              _time = true;
              _ws->textAll("Time = ON\n");
            } else if (strcmp(msg,"#TimeOFF#") == 0) {
              _time = false;
              _ws->textAll("Time = OFF \n");
            } else {
              if ((_recvFunc) && (len != 0))  _recvFunc(_context, msg);      // zero size means hear beat
            }
            
        }
    });

    _server->addHandler(_ws);
}

void WebSerialSM::setCallback(void *_ctx, RecvMsgHandler _recv, EvtConnectHandler _connect){
    _recvFunc     = _recv;
    _connectFunc  = _connect;    
    _context      = _ctx;
}

void WebSerialSM::pushLastMsg() {
  char  *head, *tail;
  short cpt, size;

  if (_buf && (!_buf->isEmpty()) ) {
    _buf->getHeadTailStrings(head, tail);  
    _ws->textAll(head);
    //Serial.printf("Available HEAD %d\n", _ws->availableForWriteAll());
    _ws->textAll(tail);
    //Serial.printf("Available TAIL %d\n", _ws->availableForWriteAll());
    _buf->reset(); 
  }
}

void WebSerialSM::addMsg(char* msg, bool store) {
  time_t    rawtime;
  struct tm ts;
  char      timeStr[12];
  
  if (!_buf && store) return;

  rawtime = _timeOffset + round(millis()/1000);  
  ts = *localtime(&rawtime);
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S - ", &ts);
  if (store) {
    if (_time) _buf->addString(timeStr);
    _buf->addString(msg); 
  } else {
   if (_time) _ws->textAll(timeStr);
   _ws->textAll(msg);
  }
}


void WebSerialSM::prints(byte prio, char *str) {
  bool send = (prio <= LOG_NOTICE) || (_debug);

  if (_isConnected)  {
    if (_ws->availableForWriteAll()) {
      pushLastMsg();

      if (send)
        if (_ws->availableForWriteAll()) addMsg(str, false);           
        else addMsg(str, true);           
    }
    else if (send) addMsg(str, true);     
  }
  else
    if (send) addMsg(str,true);     
}

void WebSerialSM::printf(char *fmt, ...) {

  va_list argp;
  va_start(argp, fmt);
  vsnprintf(_strBuf, MAX_SPRINTF_SIZE, fmt, argp);  
  va_end(argp);

  prints(LOG_NOTICE, _strBuf);
   
}


WebSerialSM WebSerial;
