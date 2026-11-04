#pragma once
#include "Arduino.h"
#define HID_REPORT_ID(x) x
#define TUD_HID_REPORT_DESC_KEYBOARD(...) 1
#define TUD_HID_REPORT_DESC_CONSUMER(...) 2
#define HID_USAGE_PAGE_N(a, b) 1
#define HID_USAGE(a) 1
#define HID_COLLECTION(a) 1
#define HID_COLLECTION_APPLICATION 1
#define HID_LOGICAL_MIN(a) 1
#define HID_LOGICAL_MAX_N(a, b) 1
#define HID_REPORT_SIZE(a) 1
#define HID_REPORT_COUNT(a) 1
#define HID_INPUT(a) 1
#define HID_OUTPUT(a) 1
#define HID_DATA 0
#define HID_VARIABLE 0
#define HID_ABSOLUTE 0
#define HID_COLLECTION_END 1
#define HID_ITF_PROTOCOL_NONE 0
typedef int hid_report_type_t;
struct Adafruit_USBD_HID { Adafruit_USBD_HID(const uint8_t*, int, int, int, bool); bool ready(); bool keyboardReport(uint8_t, uint8_t, uint8_t*); bool sendReport16(uint8_t, uint16_t); bool sendReport(uint8_t, const void*, uint8_t); void setReportCallback(void*, void (*)(uint8_t, hid_report_type_t, uint8_t const*, uint16_t)); bool begin(); };
struct TinyUSBDev { bool mounted(); bool suspended(); void detach(); void attach(); void setID(int, int); void setManufacturerDescriptor(const char*); void setProductDescriptor(const char*); }; extern TinyUSBDev TinyUSBDevice;
