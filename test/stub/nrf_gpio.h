#pragma once
#include <stdint.h>
#define NRF_GPIO_PIN_MAP(port, pin) (((port) << 5) | ((pin) & 31))
enum { NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_PULLUP };
enum { NRF_GPIO_PIN_SENSE_LOW, NRF_GPIO_PIN_SENSE_HIGH };
static inline void nrf_gpio_cfg_output(uint32_t) {}
static inline void nrf_gpio_cfg_input(uint32_t, int) {}
static inline void nrf_gpio_cfg_sense_input(uint32_t, int, int) {}
static inline void nrf_gpio_pin_set(uint32_t) {}
static inline void nrf_gpio_pin_clear(uint32_t) {}
static uint8_t stub_pin_level[64];
static inline void nrf_gpio_pin_write(uint32_t p, uint32_t v) { if (p < 64) stub_pin_level[p] = v ? 1 : 0; }
static inline uint32_t nrf_gpio_pin_read(uint32_t) { return 0; }
