#pragma once
#include <stdint.h>
#define NRF_GPIO_PIN_MAP(p, n) ((p) * 32 + (n))
#define NRF_GPIO_PIN_PULLDOWN 1
#define NRF_GPIO_PIN_SENSE_HIGH 2
void nrf_gpio_cfg_output(uint32_t); void nrf_gpio_cfg_input(uint32_t, int); void nrf_gpio_cfg_sense_input(uint32_t, int, int);
void nrf_gpio_pin_set(uint32_t); void nrf_gpio_pin_clear(uint32_t); void nrf_gpio_pin_write(uint32_t, int); int nrf_gpio_pin_read(uint32_t);
struct PowerRegs { uint32_t USBREGSTATUS; }; extern PowerRegs *NRF_POWER;
struct FicrRegs { uint32_t DEVICEADDR[2]; }; extern FicrRegs *NRF_FICR;
struct UicrRegs { uint32_t NFCPINS; }; extern UicrRegs *NRF_UICR;
struct NvmcRegs { uint32_t CONFIG, READY; }; extern NvmcRegs *NRF_NVMC;
#define POWER_USBREGSTATUS_VBUSDETECT_Msk 1u
#define UICR_NFCPINS_PROTECT_Msk 1u
#define UICR_NFCPINS_PROTECT_Pos 0
#define UICR_NFCPINS_PROTECT_NFC 1
#define NVMC_CONFIG_WEN_Pos 0
#define NVMC_CONFIG_WEN_Wen 1
#define NVMC_CONFIG_WEN_Ren 0
#define NVMC_READY_READY_Busy 0
void NVIC_SystemReset(); void __WFE(); int sd_softdevice_is_enabled(uint8_t*); int sd_power_usbregstatus_get(uint32_t*); void sd_power_system_off(); void enterUf2Dfu();
