#pragma once
#include <stdint.h>
#include <stddef.h>
namespace Adafruit_LittleFS_Namespace {
enum { FILE_O_READ = 0, FILE_O_WRITE = 1 };
struct FS_ { bool begin() { return true; } bool remove(const char *) { return true; } };
struct File { File(FS_ &) {} bool open(const char *, int) { return false; } uint32_t size() { return 0; } int read(uint8_t *, uint32_t) { return 0; } size_t write(const uint8_t *, uint32_t n) { return n; } void close() {} };
}
