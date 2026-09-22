#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>

#define ICON_COUNT 8
#define ICON_SIZE  40   // icons are ICON_SIZE x ICON_SIZE pixels, RGB565

typedef struct {
    int x;              // top-left x on screen
    int y;              // top-left y on screen
    int w;
    int h;
    const uint16_t *data; // w*h RGB565 pixels, row-major
} icon_t;

void draw_icon(const icon_t *icon);

extern icon_t icons[ICON_COUNT];

#endif // ICONS_H