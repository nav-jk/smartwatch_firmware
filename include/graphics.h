#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "specs.h"

typedef struct {
    int cx;
    int cy;
} centre;

typedef struct {
    centre cen;
    int radius;
    uint16_t color;
} circle;

void draw_circle(circle *c);
void draw_ring(circle *c, int thickness);
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void draw_line_centre(int len, float angle, uint16_t color);
void draw_rect(int x, int y, int w, int h, uint16_t color);
void draw_rect_outline(int x, int y, int w, int h, uint16_t color);
void draw_rounded_rect(int x, int y, int w, int h, int r, uint16_t color);

#endif