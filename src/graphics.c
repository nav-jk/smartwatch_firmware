#include <math.h>
#include <stdlib.h>

#include "../include/graphics.h"
#include "../include/framebuffer.h"  

void draw_circle(circle *c)
{
    for (int y = -c->radius; y <= c->radius; y++) {
        for (int x = -c->radius; x <= c->radius; x++) {
            if (x * x + y * y <= c->radius * c->radius) {
                fb_set(
                    c->cen.cx + x,
                    c->cen.cy + y,
                    c->color
                );
            }
        }
    }
}

// unfilled ring, thickness (r_outer - r_inner), for borders/bezels
void draw_ring(circle *c, int thickness)
{
    int r_out = c->radius;
    int r_in = c->radius - thickness;

    for (int y = -r_out; y <= r_out; y++) {
        for (int x = -r_out; x <= r_out; x++) {
            int d2 = x * x + y * y;
            if (d2 <= r_out * r_out && d2 >= r_in * r_in) {
                fb_set(
                    c->cen.cx + x,
                    c->cen.cy + y,
                    c->color
                );
            }
        }
    }
}

void draw_line(
    int x0,
    int y0,
    int x1,
    int y1,
    uint16_t color
)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (1) {
        fb_set(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_line_centre(
    int len,
    float angle,
    uint16_t color
)
{
    float rad = angle * 3.14159265f / 180.0f;

    int cx = LCD_H_RES / 2;
    int cy = LCD_V_RES / 2;

    int x1 = cx + (int)(len * sinf(rad));
    int y1 = cy - (int)(len * cosf(rad));

    draw_line(
        cx,
        cy,
        x1,
        y1,
        color
    );
}

// filled rectangle, top-left corner at (x, y)
void draw_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            fb_set(x + i, y + j, color);
        }
    }
}

// unfilled rectangle outline, top-left corner at (x, y)
void draw_rect_outline(int x, int y, int w, int h, uint16_t color)
{
    draw_line(x, y, x + w - 1, y, color);                 // top
    draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);  // bottom
    draw_line(x, y, x, y + h - 1, color);                  // left
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);  // right
}

// filled rounded rectangle, top-left corner at (x, y), corner radius r
void draw_rounded_rect(int x, int y, int w, int h, int r, uint16_t color)
{
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    for (int dy = 0; dy < h; dy++) {
        int inset = 0;

        if (dy < r) {
            int cy_off = r - 1 - dy;
            float dx = sqrtf((float)(r * r - cy_off * cy_off));
            inset = r - (int)dx;
        } else if (dy >= h - r) {
            int cy_off = dy - (h - r);
            float dx = sqrtf((float)(r * r - cy_off * cy_off));
            inset = r - (int)dx;
        }

        draw_line(x + inset, y + dy, x + w - 1 - inset, y + dy, color);
    }
}