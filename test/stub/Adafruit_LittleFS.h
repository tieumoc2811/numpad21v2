#pragma once
#include "Arduino.h"
namespace Adafruit_LittleFS_Namespace { enum { FILE_O_READ, FILE_O_WRITE };
struct FileSys { bool begin(); bool remove(const char*); };
struct File { File(FileSys&); bool open(const char*, int); uint32_t size(); int read(uint8_t*, uint32_t); uint32_t write(const uint8_t*, uint32_t); void close(); }; }
