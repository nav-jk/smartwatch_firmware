#include <math.h>
#include <stdlib.h>

#include "../include/clock_module.h"
#include "../include/graphics.h"


// short radial mark, offset from centre by r0 out to r1, at given angle
void draw_tick(int r0, int r1, float angle, uint16_t color)
{
    float rad = angle * 3.14159265f / 180.0f;

    int cx = LCD_H_RES / 2;
    int cy = LCD_V_RES / 2;

    int x0 = cx + (int)(r0 * sinf(rad));
    int y0 = cy - (int)(r0 * cosf(rad));
    int x1 = cx + (int)(r1 * sinf(rad));
    int y1 = cy - (int)(r1 * cosf(rad));

    draw_line(x0, y0, x1, y1, color);
}

void draw_dial_ticks(void)
{
    for (int i = 0; i < 60; i++) {
        float angle = i * 6.0f;
        if (i % 5 == 0) {
            draw_tick(96, 113, angle, 0xFFFF);
        } else {
            draw_tick(106, 113, angle, 0x39E7);
        }
    }
}

// tapered hand: a filled triangle from centre out to the tip, with a
// small back-stub for balance, plus a soft outline pass for anti-aliasing
void draw_hand(int len, int back, int width, float angle, uint16_t color)
{
    float rad = angle * 3.14159265f / 180.0f;
    float s = sinf(rad), cq = cosf(rad);
    float px = cq, py = s; // perpendicular direction

    int cx = LCD_H_RES / 2;
    int cy = LCD_V_RES / 2;

    int tipx  = cx + (int)(len * s);
    int tipy  = cy - (int)(len * cq);
    int backx = cx - (int)(back * s);
    int backy = cy + (int)(back * cq);

    float half = width * 0.5f;
    int lx = cx + (int)(half * px);
    int ly = cy + (int)(half * py);
    int rx = cx - (int)(half * px);
    int ry = cy - (int)(half * py);

    draw_line(lx, ly, tipx, tipy, color);
    draw_line(rx, ry, tipx, tipy, color);
    draw_line(lx, ly, rx, ry, color);
    draw_line(lx, ly, backx, backy, color);
    draw_line(rx, ry, backx, backy, color);

    // fill by sweeping a few interior lines from tip to base edge
    for (int i = 1; i < width; i++) {
        float t = (float)i / width - 0.5f;
        int fx = cx + (int)(t * width * px);
        int fy = cy + (int)(t * width * py);
        draw_line(fx, fy, tipx, tipy, color);
    }
}

void clock_advance(wall_clock *cur_clock)
{
    cur_clock->seconds++;

    if (cur_clock->seconds >= 60) {
        cur_clock->seconds = 0;
        cur_clock->minutes++;

        if (cur_clock->minutes >= 60) {
            cur_clock->minutes = 0;
            cur_clock->hour++;

            if (cur_clock->hour >= 24) {
                cur_clock->hour = 0;
            }
        }
    }
}

clock_angles get_clock_angles(wall_clock *cur_clock)
{
    clock_angles angles;

    angles.second_angle =
        cur_clock->seconds * 6.0f;

    angles.minute_angle =
        cur_clock->minutes * 6.0f
        + cur_clock->seconds * 0.1f;

    angles.hour_angle =
        (cur_clock->hour % 12) * 30.0f
        + cur_clock->minutes * 0.5f;

    return angles;
}

void draw_clock(wall_clock *cur_clock)
{
    clock_angles angle = get_clock_angles(cur_clock);
    draw_hand(55, 12, 7, angle.hour_angle, COLOR_HOUR);
    draw_hand(88, 16, 5, angle.minute_angle, COLOR_MINUTE);
    draw_line_centre(100, angle.second_angle, COLOR_SECOND);
    draw_tick(0, 18, angle.second_angle + 180.0f, COLOR_SECOND); // counterweight tail
}