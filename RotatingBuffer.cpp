#include "RotatingBuffer.h"
#include <string.h>
#include <stdlib.h>

RotatingBuffer::RotatingBuffer(short size, short nbMsg) {
    this->size = size;
    this->nbMsg = nbMsg+1;
    
    myBuffer = (char*) malloc(this->size + 1);
    myPos = (short*) malloc(this->nbMsg * sizeof(short));
    
    reset();
}

void    RotatingBuffer::reset() {
    for (int i=0;i<nbMsg;i++) myPos[i] = 999;
    myPos[0] = 0;

    cpt = 0;
    rollOver = false;
    cursor = 0;
    splitPos = -1;
    
    memset(myBuffer, 0, size+1);
    _isEmpty = true;
}

void    RotatingBuffer::addString(char  *str) {
    short ind;
    _isEmpty = false;
            
    if (myPos[cpt] + strlen(str) < size) {
        
        if (cpt+1 == nbMsg) {
            for (short i=1;i<nbMsg;i++) myPos[i-1] = myPos[i];
            myPos[cpt] = 999;
            cpt -= 1;
        }
        
        strncpy(myBuffer+myPos[cpt],str,strlen(str));
        myPos[cpt+1] = myPos[cpt] + strlen(str) ;
        
        if (rollOver) {
            for (ind = 0; ind < cpt; ind++) {
                if (myPos[ind] > myPos[cpt+1] ) break;
                if ((ind) && (myPos[ind-1] > myPos[ind])) break;
            }
            if (ind >= cpt) ind = 0;
            
            
            for (int i=0;i<ind;i++)         // remove splited part if any
                if (myPos[i] == splitPos) {
                    ind++;
                    splitPos = -1;
                    break;
                }

            
            for (short i=ind;i<nbMsg;i++) myPos[i-ind] = myPos[i];
            cpt -= ind;
        }
        
        if (myPos[cpt+1] == size) {
            rollOver = true;
            myPos[cpt+1] = 0;
        }
        cpt++;
        
    } else { // splited in two parts
        rollOver = true;

        if (cpt+2 >= nbMsg) {
            for (short i=2;i<nbMsg;i++) myPos[i-2] = myPos[i];
            cpt -= 2;
            myPos[cpt+1] = 999;
            myPos[cpt+2] = 999;
        }
        
        short splitPt = size - myPos[cpt] ;
        strncpy(myBuffer+myPos[cpt], str, splitPt);
        strncpy(myBuffer, str+splitPt, strlen(str)-splitPt);
        splitPos = myPos[cpt];
        
        myPos[cpt+1]= 0;
        cpt++;
        
        myPos[cpt+1] = strlen(str)-splitPt;
         
        for (ind = cpt-1; ind > 1; ind--) {
            if (myPos[ind-1] <= myPos[cpt+1] ) break;
        }

        for (short i=ind;i<nbMsg;i++) myPos[i-ind] = myPos[i];
        cpt -= ind;
 
        cpt++;
    }
}

void    RotatingBuffer::getHeadTailStrings(char* &head, char* &tail) {
    short pos;
    short headSize = 0;
    short tailSize = 0;
    

    for (pos=cpt-1;pos>0 ; pos--) {
        if (myPos[pos] > myPos[pos+1]) break;
     }
  
    if (splitPos != -1)
        headSize = size-myPos[0];
    else
        headSize = myPos[cpt]-myPos[0];
    
    head = myBuffer+myPos[0];
    head[headSize] = '\0';
    
    tail = &myBuffer[size];
    
    if (splitPos != -1) {
        tailSize =(myPos[cpt]-myPos[pos+1]);
        tail = myBuffer+myPos[pos+1];
        
        //if (tailSize+headSize == size) tailSize--;
        tail[tailSize] = '\0';
        
    }
}

void    RotatingBuffer::getAllStrings(char* str, short *pSize) {
    short pos;
    short headSize = 0;
    short tailSize = 0;
    

    for (pos=cpt-1;pos>0 ; pos--) {
        if (myPos[pos] > myPos[pos+1]) break;
     }
  
    if (splitPos != -1)
        headSize = size-myPos[0];
    else
        headSize = myPos[cpt]-myPos[0];
    
    if (str) {
        strncpy(str,myBuffer+myPos[0], headSize);
        str[headSize] = '\0';
    }

    if (splitPos != -1) {
        tailSize =(myPos[cpt]-myPos[pos+1]);
        if (str) {
            strncat(str, myBuffer+myPos[pos+1],tailSize);
            str[headSize+tailSize] = '\0';
        }
    }
    
    if (pSize)  *pSize  = headSize+tailSize;
}

void    RotatingBuffer::getStatus(short &myCpt, short &mySize) {
    getAllStrings(NULL, &mySize);
    myCpt= this->cpt;
}


short   RotatingBuffer::getNextString(char*  str, bool reset) {
    short strSizeHead = 0;
    short strSizeTail = 0;
    
    if (reset) {
        cursor = 0;
        if (str)  *str = '\0';
        
        return 0;
    } else {
        if (cursor == cpt) {
            if (str) *str = '\0';
            
            cursor = 0;
            return 0;
        } else  {       
            strSizeHead=myPos[cursor+1]-myPos[cursor];
            if (strSizeHead<0)
                strSizeHead=size-myPos[cursor];

            if (str) {
              strncpy(str,myBuffer+myPos[cursor],strSizeHead);
              str[strSizeHead] = '\0';
            }
                       
            if (myPos[cursor] == splitPos) {
                cursor++;
                strSizeTail=myPos[cursor+1]-myPos[cursor];
                
                if ((str) && (strSizeTail>0)) {
                    strncat(str,myBuffer+myPos[cursor],strSizeTail);
                    str[strSizeHead+strSizeTail] = '\0';
                }
            }

            cursor++;
            return strSizeHead+strSizeTail;
        }
    }
}
