#include "../include/icons.h"
#include "../include/framebuffer.h"
#include "../include/specs.h"
#include "../assets/icons/icon_assets.h"

void draw_icon(const icon_t *icon)
{
    for (int j = 0; j < icon->h; j++) {
        for (int i = 0; i < icon->w; i++) {
            fb_set(
                icon->x + i,
                icon->y + j,
                icon->data[j * icon->w + i]
            );
        }
    }
}

// grid matching menu.c's dividing lines at y=50 and y=190:
// row 1 sits just under the top line, row 2 sits flush against the
// bottom line, four 40x40 icons per row with even spacing
#define ROW1_Y  60
#define ROW2_Y  130
#define COL0_X  30
#define COL1_X  85
#define COL2_X  140
#define COL3_X  195

icon_t icons[ICON_COUNT] = {
    { .x = COL0_X, .y = ROW1_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon0_data },
    { .x = COL1_X, .y = ROW1_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon1_data },
    { .x = COL2_X, .y = ROW1_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon2_data },
    { .x = COL3_X, .y = ROW1_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon3_data },
    { .x = COL0_X, .y = ROW2_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon4_data },
    { .x = COL1_X, .y = ROW2_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon5_data },
    { .x = COL2_X, .y = ROW2_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon6_data },
    { .x = COL3_X, .y = ROW2_Y, .w = ICON_SIZE, .h = ICON_SIZE, .data = icon7_data },
};


