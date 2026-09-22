#ifndef CLOCK_MODULE_H
#define CLOCK_MODULE_H

#include <stdint.h>

#include "specs.h"
#include "graphics.h"
#include "framebuffer.h"

#define COLOR_HOUR    0x07FF // cyan
#define COLOR_MINUTE  0xFFFF // white
#define COLOR_SECOND  0xF800 // red
#define COLOR_CAP     0xFD20 // amber


typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
} wall_clock;

typedef struct {
    float hour_angle;
    float minute_angle;
    float second_angle;
} clock_angles;

// primitives
void draw_tick(int r0, int r1, float angle, uint16_t color);
void draw_hand(int len, int back, int width, float angle, uint16_t color);

// dial / clock face
void draw_dial_ticks(void);
void draw_clock(wall_clock *cur_clock);

// time state
void clock_advance(wall_clock *cur_clock);
clock_angles get_clock_angles(wall_clock *cur_clock);

#endif // CLOCK_MODULE_H