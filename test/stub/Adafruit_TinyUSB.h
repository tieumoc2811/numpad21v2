#pragma once
#include <stdint.h>
#define TUD_HID_REPORT_DESC_KEYBOARD(...) 0
#define TUD_HID_REPORT_DESC_CONSUMER(...) 0
#define HID_REPORT_ID(...) 0
#define HID_USAGE_PAGE_N(...) 0
#define HID_USAGE(...) 0
#define HID_COLLECTION(...) 0
#define HID_COLLECTION_APPLICATION 0
#define HID_COLLECTION_END 0
#define HID_LOGICAL_MIN(...) 0
#define HID_LOGICAL_MAX_N(...) 0
#define HID_REPORT_SIZE(...) 0
#define HID_REPORT_COUNT(...) 0
#define HID_INPUT(...) 0
#define HID_OUTPUT(...) 0
#define HID_DATA 0
#define HID_VARIABLE 0
#define HID_ABSOLUTE 0
#define HID_ITF_PROTOCOL_NONE 0
typedef int hid_report_type_t;
struct Adafruit_USBD_HID {
  Adafruit_USBD_HID(const uint8_t *, uint16_t, int, uint8_t, bool) {}
  void begin() {} bool ready(); bool keyboardReport(uint8_t id, uint8_t mod, uint8_t *k); bool sendReport16(uint8_t id, uint16_t v); bool sendReport(uint8_t, const void *, uint8_t) { return true; }
  void setReportCallback(void *, void (*)(uint8_t, hid_report_type_t, uint8_t const *, uint16_t)) {}
};
struct TinyUSBDeviceClass { bool mounted(); bool suspended(); void detach() {} void attach() {} void setID(uint16_t, uint16_t) {} void setManufacturerDescriptor(const char *) {} void setProductDescriptor(const char *) {} };
extern TinyUSBDeviceClass TinyUSBDevice;
