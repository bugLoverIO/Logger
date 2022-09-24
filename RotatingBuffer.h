#ifndef  _ROTATING_BUFFER
#define _ROTATING_BUFFER
#include <Arduino.h>

class RotatingBuffer {
private:
    char*   myBuffer;       // main buffer
    short*  myPos;          // position buffer
    char    nbMsg;          // max nb msg
    short   size;           // main buffer size
    bool    rollOver;       // position rollover
    char    cpt;            // last available position in myPos buffer
    char    cursor;         // reading position
    short   splitPos;       // Split Position 
    bool    _isEmpty;

public:
            RotatingBuffer(short size, short nbMsg);
    void    reset();
    void    addString(char* str);                                         // copy string to buffer    
    void    getStatus(short &cpt, short &size);
    short   getNextString(char* str,  bool reset = false);                // get nextString and return its size, set str=NULL to get string size only.
    void    getAllStrings(char* str, short *pSize=NULL);                  // set str=NULL to get string size
    void    getHeadTailStrings(char* &head, char* &tail);
    bool    isEmpty() {return _isEmpty;};

};

#endif
