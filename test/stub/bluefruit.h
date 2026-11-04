#pragma once
#include <stdint.h>
#include "nrf_gpio.h"
struct ble_gap_addr_t { uint8_t addr_type; uint8_t addr[6]; };
#define BLE_GAP_ADDR_TYPE_RANDOM_STATIC 1
#define BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE 6
#define BLE_APPEARANCE_HID_KEYBOARD 961
#define POWER_USBREGSTATUS_VBUSDETECT_Msk 1u
struct nrf_power_t { uint32_t USBREGSTATUS; }; extern nrf_power_t nrf_power_o;
#define NRF_POWER (&nrf_power_o)
struct nrf_ficr_t { uint32_t DEVICEADDR[2]; }; extern nrf_ficr_t nrf_ficr_o;
#define NRF_FICR (&nrf_ficr_o)
struct nrf_uicr_t { uint32_t NFCPINS; }; extern nrf_uicr_t nrf_uicr_o;
#define NRF_UICR (&nrf_uicr_o)
#define UICR_NFCPINS_PROTECT_Msk 1u
#define UICR_NFCPINS_PROTECT_Pos 0
#define UICR_NFCPINS_PROTECT_NFC 1u
struct nrf_nvmc_t { uint32_t CONFIG, READY; }; extern nrf_nvmc_t nrf_nvmc_o;
#define NRF_NVMC (&nrf_nvmc_o)
#define NVMC_CONFIG_WEN_Wen 1
#define NVMC_CONFIG_WEN_Ren 0
#define NVMC_CONFIG_WEN_Pos 0
#define NVMC_READY_READY_Busy 0
static inline void NVIC_SystemReset(void) {}
static inline void __WFE(void) {}
static inline int sd_softdevice_is_enabled(uint8_t *e) { *e = 0; return 0; }
static inline int sd_power_usbregstatus_get(uint32_t *s) { *s = 0; return 0; }
static inline int sd_power_system_off(void) { return 0; }
void enterUf2Dfu(void);
struct BLEAdv {
  void stop() {} bool isRunning() { return false; } void start(int) {} void clearData() {} void addFlags(int) {} void addTxPower() {} void addAppearance(int) {}
  template <class T> void addService(T &) {} void addName() {} void restartOnDisconnect(bool) {} void setInterval(int, int) {} void setFastTimeout(int) {}
};
struct BLEPeriph { void setConnInterval(int, int) {} void setConnectCallback(void (*)(uint16_t)) {} void setDisconnectCallback(void (*)(uint16_t, uint8_t)) {} void clearBonds() {} };
struct BluefruitClass {
  BLEAdv Advertising, ScanResponse; BLEPeriph Periph;
  void autoConnLed(bool) {} void begin() {} void setTxPower(int) {} ble_gap_addr_t getAddr() { return ble_gap_addr_t(); } void setAddr(ble_gap_addr_t *) {} void setName(const char *) {} void disconnect(uint16_t) {}
};
extern BluefruitClass Bluefruit;
struct BLEDis { void setManufacturer(const char *) {} void setModel(const char *) {} void begin() {} };
struct BLEHidAdafruit { void begin() {} bool keyboardReport(uint8_t mod, uint8_t k[6]); bool consumerReport(uint16_t c); };
