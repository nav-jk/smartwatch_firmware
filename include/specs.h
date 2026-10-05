#ifndef SPECS_H
#define SPECS_H

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define WIFI_SSID "iPhone"
#define WIFI_PASS "12345678"
#define TZ_STRING "IST-5:30"

#define PIN_SCLK 22
#define PIN_MOSI 21
#define PIN_CS 2
#define PIN_DC 4
#define PIN_RST 23
#define PIN_BL -1

#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIXEL_CLK_HZ (40 * 1000 * 1000)

#define LCD_H_RES 240
#define LCD_V_RES 240
#define DRAW_BUF_LINES 40

#define ROT_SWAP_XY false
#define ROT_MIRROR_X false
#define ROT_MIRROR_Y true

#define PIN_SWITCH_BTN GPIO_NUM_0
#define LONG_PRESS_MS 600

#endif
