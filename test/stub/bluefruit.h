#pragma once
#include "Arduino.h"
#define BLE_GAP_ADDR_TYPE_RANDOM_STATIC 1
#define BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE 6
#define BLE_APPEARANCE_HID_KEYBOARD 961
struct ble_gap_addr_t { uint8_t addr_type; uint8_t addr[6]; };
struct BLEHidAdafruit { bool begin(); bool keyboardReport(uint8_t, uint8_t[6]); bool consumerReport(uint16_t); };
struct BLEDis { void setManufacturer(const char*); void setModel(const char*); bool begin(); };
struct Adv { void stop(); void clearData(); void addFlags(uint8_t); void addTxPower(); void addAppearance(uint16_t); void addService(BLEHidAdafruit&); void restartOnDisconnect(bool); void setInterval(int, int); void setFastTimeout(int); bool isRunning(); bool start(int); };
struct Scan { void clearData(); void addName(); };
struct PeriphT { void clearBonds(); void setConnInterval(int, int); void setConnectCallback(void (*)(uint16_t)); void setDisconnectCallback(void (*)(uint16_t, uint8_t)); };
struct BluefruitClass { Adv Advertising; Scan ScanResponse; PeriphT Periph; ble_gap_addr_t getAddr(); bool setAddr(ble_gap_addr_t*); bool setName(const char*); void autoConnLed(bool); bool begin(); void setTxPower(int); bool disconnect(uint16_t); };
extern BluefruitClass Bluefruit;
