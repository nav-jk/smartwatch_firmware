#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

// draws a single character at (x, y) [top-left of glyph cell], scaled by `scale`
// (1 = native 5x7 px, 2 = 10x14 px, etc). Returns the glyph cell width in px
// (5*scale) so callers can advance a cursor.
int draw_char(int x, int y, char c, uint16_t color, int scale);

// draws `text` word-wrapped and clipped inside the box (x, y, w, h).
// Lines that would overflow the box height are simply not drawn (clipped),
// rather than scrolling — call again with a scrolled offset if you need that.
void draw_text_box(int x, int y, int w, int h, const char *text, uint16_t color, int scale);

#endif // TEXT_H