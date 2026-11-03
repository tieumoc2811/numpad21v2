// board.h - so do chan Numpad21 (Pro Micro nRF52840 / nice!nano v2). GIU NGUYEN phan cung cua ban ZMK cu.
// Dung so chan nRF truc tiep (P0.xx / P1.xx), khong phu thuoc bang chan Arduino cua variant.
#pragma once
#include <stdint.h>
#include "nrf_gpio.h"

#define P(port, pin) NRF_GPIO_PIN_MAP(port, pin)

// Ma tran 7x7 co diode. 0 = col2row (cot la output, hang la input pull-down; giong overlay ZMK cu)
//                       1 = row2col (hang la output, cot la input pull-down) - doi neu diode lap nguoc
#ifndef DIODE_ROW2COL
#define DIODE_ROW2COL 0
#endif

#define NROW 7
#define NCOL 7
static const uint8_t COLP[NCOL] = { P(0, 11), P(1, 4), P(0, 31), P(0, 29), P(0, 2), P(1, 15), P(1, 13) };   // C0..C6
static const uint8_t ROWP[NROW] = { P(0, 6), P(0, 8), P(0, 17), P(0, 20), P(0, 22), P(0, 24), P(1, 0) };     // R0..R6

// 22 vi tri phim (21 phim + nut chuyen layer nho), theo thu tu tren->xuong, trai->phai:
//   ESC  TAB  BSPC FN          0  1  2  3
//   NUM  /    *    -           4  5  6  7
//   7    8    9                8  9  10
//   4    5    6    +           11 12 13 14
//   1    2    3    ENTER       15 16 17 18
//   0    .                     19 20
//   (nut chuyen layer nho)     21
#define NK 22
static const uint8_t KROW[NK] = { 4, 6, 5, 4,  3, 3, 3, 3,  2, 2, 2,  1, 1, 1, 2,  0, 0, 0, 0,  4, 4,  6 };
static const uint8_t KCOL[NK] = { 4, 4, 6, 5,  0, 1, 2, 3,  0, 1, 2,  0, 1, 2, 3,  0, 1, 2, 3,  1, 2,  5 };

// 6 day LED qua transistor PNP (S8550), base active-low (LOW = sang). LED0..LED5
#define NLED 6
static const uint8_t LEDP[NLED] = { P(0, 9), P(0, 10), P(1, 1), P(1, 2), P(1, 7), P(1, 11) };
