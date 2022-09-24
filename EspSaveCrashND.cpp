/*
  This in an Arduino library to save exception details
  and stack trace to flash in case of ESP8266 crash.
  Please check repository below for details

  Repository: https://github.com/krzychb/EspSaveCrash
  File: EspSaveCrash.cpp
  Revision: 1.1.0
  Date: 18-Aug-2016
  Author: krzychb at gazeta.pl

  Copyright (c) 2016 Krzysztof Budzynski. All rights reserved.

  This application is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This application is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "EspSaveCrashND.h"
#include <time.h>

/**
 * EEPROM layout
 *
 * Note that for using EEPROM we are also reserving a RAM buffer
 * The buffer size will be bigger by static uint16_t _offset than what we actually need
 * The space that we really need is defined by static uint16_t _size
 */
uint16_t EspSaveCrash::_offset     = 0x0010;
uint16_t EspSaveCrash::_size       = 0x0200;
uint32_t EspSaveCrash::_timeOffset = 0;

/**
 * Save crash information in EEPROM
 * This function is called automatically if ESP8266 suffers an exception
 * It should be kept quick / concise to be able to execute before hardware wdt may kick in
 */
extern "C" void custom_crash_callback(struct rst_info * rst_info, uint32_t stack, uint32_t stack_end )
{
  // Note that 'EEPROM.begin' method is reserving a RAM buffer
  // The buffer size is SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_SPACE_SIZE
  EEPROM.begin(EspSaveCrash::_offset + EspSaveCrash::_size);

  byte crashCounter = EEPROM.read(EspSaveCrash::_offset + SAVE_CRASH_COUNTER);
  int16_t writeFrom;
  if(crashCounter == 0)
  {
    writeFrom = SAVE_CRASH_DATA_SETS;
  }
  else
  {
    EEPROM.get(EspSaveCrash::_offset + SAVE_CRASH_WRITE_FROM, writeFrom);
  }

  // is there free EEPROM space available to save data for this crash?
  if (writeFrom + SAVE_CRASH_STACK_TRACE > EspSaveCrash::_size)
  {
    return;
  }

  // increment crash counter and write it to EEPROM
  EEPROM.write(EspSaveCrash::_offset + SAVE_CRASH_COUNTER, ++crashCounter);

  // now address EEPROM contents including _offset
  writeFrom += EspSaveCrash::_offset;

  // write crash time to EEPROM
  uint32_t crashTime = EspSaveCrash::_timeOffset + (int)(millis()/1000);
  EEPROM.put(writeFrom + SAVE_CRASH_CRASH_TIME, crashTime);

  // write reset info to EEPROM
  EEPROM.write(writeFrom + SAVE_CRASH_RESTART_REASON, rst_info->reason);
  EEPROM.write(writeFrom + SAVE_CRASH_EXCEPTION_CAUSE, rst_info->exccause);

  // write stack start and end address to EEPROM
  EEPROM.put(writeFrom + SAVE_CRASH_STACK_START, stack);
  EEPROM.put(writeFrom + SAVE_CRASH_STACK_END, stack_end);

  // write stack trace to EEPROM
  int16_t currentAddress = writeFrom + SAVE_CRASH_STACK_TRACE;
  for (uint32_t iAddress = stack; iAddress < stack_end; iAddress++)
  {
    byte* byteValue = (byte*) iAddress;
    EEPROM.write(currentAddress++, *byteValue);
    if (currentAddress - EspSaveCrash::_offset > EspSaveCrash::_size)
    {
      // ToDo: flag an incomplete stack trace written to EEPROM!
      break;
    }
  }
  // now exclude _offset from address written to EEPROM
  currentAddress -= EspSaveCrash::_offset;
  EEPROM.put(EspSaveCrash::_offset + SAVE_CRASH_WRITE_FROM, currentAddress);

  EEPROM.commit();
}


/**
 * The class constructor
 */
EspSaveCrash::EspSaveCrash(uint16_t off, uint16_t size)
{
  _offset = off;
  _size = size;
}

/**
 * Clear crash information saved in EEPROM
 * In fact only crash counter is cleared
 * The crash data is not deleted
 */
void EspSaveCrash::clear(void)
{
  // Note that 'EEPROM.begin' method is reserving a RAM buffer
  // The buffer size is SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_SPACE_SIZE
  EEPROM.begin(_offset + _size);
  // clear the crash counter
  EEPROM.write(_offset + SAVE_CRASH_COUNTER, 0);
  EEPROM.end();
}


/**
 * Print out crash information that has been previously saved in EEPROM
 * @param outputDev Print&    Optional. Where to print: Serial, Serial, WiFiClient, etc.
 */
void EspSaveCrash::print(Print& outputDev)
{

  time_t    rawtime;
  struct tm ts;
  char      buf[80];
  byte      reason;  
 
  // Note that 'EEPROM.begin' method is reserving a RAM buffer
  // The buffer size is SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_SPACE_SIZE
  EEPROM.begin(_offset + _size);
  byte crashCounter = EEPROM.read(_offset + SAVE_CRASH_COUNTER);
  outputDev.println("- - - - - - - - - - - - - - - - - - - - - - - - - -");
  if (crashCounter == 0)
  {
    outputDev.println("No crash saved");
    outputDev.println("- - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    return;
  }

  outputDev.println("Crash information recovered from EEPROM");
  int16_t readFrom = _offset + SAVE_CRASH_DATA_SETS;
  for (byte k = 0; k < crashCounter; k++)
  {
    uint32_t crashTime;
    EEPROM.get(readFrom + SAVE_CRASH_CRASH_TIME, crashTime);
    
    rawtime = crashTime;
    ts = *localtime(&rawtime );
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
    
    outputDev.printf("Crash # %d at %s\n", k + 1, buf);

    reason = EEPROM.read(readFrom + SAVE_CRASH_RESTART_REASON);
    switch (reason ) {
      case REASON_DEFAULT_RST:      strcpy(buf,"REASON_DEFAULT_RST - normal startup by power on");     break;
      case REASON_WDT_RST:          strcpy(buf,"REASON_WDT_RST - hardware watch dog reset");           break;
      case REASON_EXCEPTION_RST:    strcpy(buf,"REASON_EXCEPTION_RST - exception reset");              break;
      case REASON_SOFT_WDT_RST:     strcpy(buf,"REASON_SOFT_WDT_RST - software watch dog reset");      break;
      case REASON_SOFT_RESTART:     strcpy(buf,"REASON_SOFT_RESTART - software restart");              break;
      case REASON_DEEP_SLEEP_AWAKE: strcpy(buf,"REASON_DEEP_SLEEP_AWAKE - wake up from deep-sleep");   break;
      case REASON_EXT_SYS_RST:      strcpy(buf,"REASON_EXT_SYS_RST - external system reset");          break;
      default :                     strcpy(buf,"REASON_UNKNOWN - unknown reason");       
    }
    outputDev.printf("Restart reason: %d %s\n", reason ,buf);

    if (reason == REASON_EXCEPTION_RST) {
      switch (EEPROM.read(readFrom + SAVE_CRASH_EXCEPTION_CAUSE)) {
        case ( 0) : strcpy(buf,"IllegalInstructionCause"); break;
        case ( 1) : strcpy(buf,"SyscallCause"); break;
        case ( 2) : strcpy(buf,"InstructionFetchErrorCause"); break;
        case ( 3) : strcpy(buf,"LoadStoreErrorCause"); break;
        case ( 4) : strcpy(buf,"Level1InterruptCause"); break;
        case ( 5) : strcpy(buf,"AllocaCause"); break;
        case ( 6) : strcpy(buf,"IntegerDivideByZeroCause"); break;
        case ( 7) : strcpy(buf,"Reserved for Tensilica"); break;
        case ( 8) : strcpy(buf,"PrivilegedCause"); break;
        case ( 9) : strcpy(buf,"LoadStoreAlignmentCause"); break;
        case (10) :
        case (11) : strcpy(buf,"Reserved for Tensilica"); break;
        case (12) : strcpy(buf,"InstrPIFDataErrorCause"); break;
        case (13) : strcpy(buf,"LoadStorePIFDataErrorCause"); break;
        case (14) : strcpy(buf,"InstrPIFAddrErrorCause"); break;
        case (15) : strcpy(buf,"LoadStorePIFAddrErrorCause"); break;
        case (16) : strcpy(buf,"InstTLBMissCause"); break;
        case (17) : strcpy(buf,"InstTLBMultiHitCause"); break;
        case (18) : strcpy(buf,"InstFetchPrivilegeCause"); break;
        case (19) : strcpy(buf,"Reserved for Tensilica"); break;
        case (20) : strcpy(buf,"InstFetchProhibitedCause"); break;
        case (21) :
        case (23) : strcpy(buf,"Reserved for Tensilica"); break;
        case (24) : strcpy(buf,"LoadStoreTLBMissCause"); break;
        case (25) : strcpy(buf,"LoadStoreTLBMultiHitCause"); break;
        case (26) : strcpy(buf,"LoadStorePrivilegeCause"); break;
        case (27) : strcpy(buf,"Reserved for Tensilica"); break;
        case (28) : strcpy(buf,"LoadProhibitedCause"); break;
        case (29) : strcpy(buf,"StoreProhibitedCause"); break;
        default :   strcpy(buf,"Reserved"); break;    
      }
      outputDev.printf("Exception cause: %d - %s\n", EEPROM.read(readFrom + SAVE_CRASH_EXCEPTION_CAUSE),buf);
    }

    uint32_t stackStart, stackEnd;
    EEPROM.get(readFrom + SAVE_CRASH_STACK_START, stackStart);
    EEPROM.get(readFrom + SAVE_CRASH_STACK_END, stackEnd);
    outputDev.printf(">>>stack>>>\n");
    int16_t currentAddress = readFrom + SAVE_CRASH_STACK_TRACE;
    int16_t stackLength = stackEnd - stackStart;
    uint32_t stackTrace;
    for (int16_t i = 0; i < stackLength; i += 0x10)
    {
      outputDev.printf("%08x: ", stackStart + i);
      for (byte j = 0; j < 4; j++)
      {
        EEPROM.get(currentAddress, stackTrace);
        outputDev.printf("%08x ", stackTrace);
        currentAddress += 4;
        if (currentAddress - _offset > _size)
        {
          outputDev.println("\nIncomplete stack trace saved!");
          goto eepromSpaceEnd;
        }
      }
      outputDev.printf("\n");
    }
    eepromSpaceEnd:
    outputDev.printf("<<<stack<<<\n");
    readFrom = readFrom + SAVE_CRASH_STACK_TRACE + stackLength;
  }
  int16_t writeFrom;
  EEPROM.get(_offset + SAVE_CRASH_WRITE_FROM, writeFrom);
  EEPROM.end();

  // is there free EEPROM space available to save data for next crash?
  if (writeFrom + SAVE_CRASH_STACK_TRACE > _size)
  {
    outputDev.println("No more EEPROM space available to save crash information!");
  }
  else
  {
    outputDev.printf("EEPROM space available: 0x%04x bytes\n", _size - writeFrom);
  }
  outputDev.println("- - - - - - - - - - - - - - - - - - - - - - - - - -\n");
}

/**
 * Write crash information that has been previously saved in EEPROM to user buffer.
 * @param userBuffer User-owned buffer
 * @param size       Length of user-owned buffer
 * @return Number of characters written to the buffer
 */
size_t EspSaveCrash::print(char* userBuffer, size_t size)
{
  struct BufferPrinter : Print {
    BufferPrinter(char* buffer, size_t size) : _buffer{buffer}, _bufferSize{size} {}

    size_t write(uint8_t ch) override {
      return write(&ch, 1);
    }

    size_t write(const uint8_t *buffer, size_t size) override {
      size_t writeSize = _pos + size <= _bufferSize ? size : _bufferSize - _pos;
      memcpy(_buffer + _pos, buffer, writeSize);
      _pos += writeSize;
      return writeSize;
    }

    char *_buffer;
    size_t _bufferSize;
    size_t _pos{0};
  };

  BufferPrinter printer(userBuffer, size);
  print(printer);
  printer.write('\0');
  return printer._pos;
}

/**
 * @brief      DEPRECATED Set crash information that has been previously saved in EEPROM to user buffer
 * @param      userBuffer  The user buffer
 */
void EspSaveCrash::crashToBuffer(char* userBuffer)
{
  print(userBuffer, SIZE_MAX);
}

/**
 * Get the count of crash data sets saved in EEPROM
 */
int EspSaveCrash::count()
{
  EEPROM.begin(_offset + _size);
  int crashCounter = EEPROM.read(_offset + SAVE_CRASH_COUNTER);
  EEPROM.end();
  return crashCounter;
}

/**
 * Get the memory offset value
 */
uint16_t EspSaveCrash::offset()
{
  return _offset;
}

/**
 * Get the memory size value
 */
uint16_t EspSaveCrash::size()
{
  return _size;
}
