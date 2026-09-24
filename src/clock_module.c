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

// fills a simple (convex, non-self-intersecting) polygon given its vertices,
// via a standard scanline edge-intersection fill
static void draw_polygon(const int *px, const int *py, int n, uint16_t color)
{
    int ymin = py[0], ymax = py[0];
    for (int i = 1; i < n; i++) {
        if (py[i] < ymin) ymin = py[i];
        if (py[i] > ymax) ymax = py[i];
    }

    for (int y = ymin; y <= ymax; y++) {
        int xs[8];
        int count = 0;

        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            int y0 = py[i], y1 = py[j];
            int x0 = px[i], x1 = px[j];

            if (y0 == y1) continue;
            if ((y >= y0 && y < y1) || (y >= y1 && y < y0)) {
                float t = (float)(y - y0) / (float)(y1 - y0);
                xs[count++] = x0 + (int)(t * (x1 - x0));
            }
        }

        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (xs[j] < xs[i]) {
                    int tmp = xs[i]; xs[i] = xs[j]; xs[j] = tmp;
                }
            }
        }

        for (int i = 0; i + 1 < count; i += 2) {
            for (int x = xs[i]; x <= xs[i + 1]; x++) {
                fb_set(x, y, color);
            }
        }
    }
}

// builds a simple straight bar hand: constant width along its length,
// tapering to a point only at the very tip, with a short flat back past
// the pivot. This reads as a bold, plain needle (like a typical entry
// level watch-face hand) rather than a faceted dauphine kite.
static void build_hand_bar(int len, int back, int width, float angle, int *px, int *py)
{
    float rad = angle * 3.14159265f / 180.0f;
    float s = sinf(rad), cq = cosf(rad);
    float perp_x = cq, perp_y = s;

    int cx = LCD_H_RES / 2;
    int cy = LCD_V_RES / 2;

    float half = width * 0.5f;

    int tipx  = cx + (int)(len * s);
    int tipy  = cy - (int)(len * cq);

    int backx = cx - (int)(back * s);
    int backy = cy + (int)(back * cq);

    int lx = backx + (int)(half * perp_x);
    int ly = backy + (int)(half * perp_y);
    int rx = backx - (int)(half * perp_x);
    int ry = backy - (int)(half * perp_y);

    px[0] = tipx;  py[0] = tipy;
    px[1] = rx;    py[1] = ry;
    px[2] = backx; py[2] = backy;
    px[3] = lx;    py[3] = ly;
}

// flat, wide, plain-white hand, outlined for contrast against the dial:
// a slightly larger dark silhouette is drawn first, then the actual hand
// color on top, leaving a crisp ~2px border all the way around so the
// hand stays legible against the busy sub-dial background.
void draw_hand(int len, int back, int width, float angle, uint16_t color)
{
    int px[4], py[4];

    build_hand_bar(len + 2, back + 2, width + 4, angle, px, py);
    draw_polygon(px, py, 4, 0x0000); // dark outline silhouette

    build_hand_bar(len, back, width, angle, px, py);
    draw_polygon(px, py, 4, color);  // actual hand fill on top
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
    draw_hand(55, 8, 16, angle.hour_angle, COLOR_HOUR);
    draw_hand(88, 8, 14, angle.minute_angle, COLOR_MINUTE);
    draw_line_centre(100, angle.second_angle, COLOR_SECOND);
    draw_tick(0, 18, angle.second_angle + 180.0f, COLOR_SECOND); // counterweight tail
}