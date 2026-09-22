#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

#define LCD_H_RES  240
#define LCD_V_RES  240

#define COLOR_HOUR    0x07FF // cyan
#define COLOR_MINUTE  0xFFFF // white
#define COLOR_SECOND  0xF800 // red
#define COLOR_CAP     0xFD20 // amber

typedef struct {
    int cx;
    int cy;
} centre;

typedef struct {
    centre cen;
    int radius;
    uint16_t color;
} circle;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
} clock;

typedef struct {
    float hour_angle;
    float minute_angle;
    float second_angle;
} clock_angles;

// provided by main.c: writes a single pixel into the shared framebuffer
void fb_set(int x, int y, uint16_t color);

// primitives
void draw_circle(circle *c);
void draw_ring(circle *c, int thickness);
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void draw_line_centre(int len, float angle, uint16_t color);
void draw_tick(int r0, int r1, float angle, uint16_t color);
void draw_hand(int len, int back, int width, float angle, uint16_t color);

// dial / clock face
void draw_dial_ticks(void);
void draw_clock(clock *cur_clock);

// time state
void clock_advance(clock *cur_clock);
clock_angles get_clock_angles(clock *cur_clock);

#endif // CLOCK_H