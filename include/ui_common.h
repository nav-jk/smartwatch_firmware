#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdbool.h>
#include "lvgl.h"

/* ---- Palette (HyperOS-inspired: pure black, warm orange accent) ---- */
#define COL_BG        lv_color_hex(0x000000)
#define COL_TEXT      lv_color_hex(0xFFFFFF)
#define COL_MUTED     lv_color_hex(0x8E939B)
#define COL_ACCENT    lv_color_hex(0xFF7A3D)
#define COL_TRACK     lv_color_hex(0x17191D)
#define COL_SECOND    lv_color_hex(0xFF7A3D)
#define COL_DARK      lv_color_hex(0x1C1C1E)
#define COL_CARD      lv_color_hex(0x16181C)
#define COL_TICK      lv_color_hex(0x3A3F47)
#define COL_GREEN     lv_color_hex(0x30D158)
#define COL_PINK      lv_color_hex(0xFF375F)

/* Remove click + scroll behaviour so swipes pass through to the tileview */
void ui_make_inert(lv_obj_t *obj);

lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color,
                   const char *text, lv_align_t align, int x, int y);

/* Solid rounded rectangle / circle, positioned from the parent's centre */
lv_obj_t *ui_shape(lv_obj_t *parent, int w, int h, int radius, lv_color_t col,
                   int dx, int dy);

/* Hollow circle / pill outline */
lv_obj_t *ui_outline(lv_obj_t *parent, int w, int h, int border, lv_color_t col,
                     int dx, int dy);

/* Thin progress ring (arc) */
lv_obj_t *ui_ring(lv_obj_t *parent, int size, int width, lv_color_t color,
                  lv_color_t track, int range_max);

/* Dark capsule with a coloured dot and a value; returns the value label */
lv_obj_t *ui_pill(lv_obj_t *parent, int w, int h, int dx, int dy,
                  lv_color_t dot, const char *text);

/* Centered multi-line text body used by the simple app pages */
lv_obj_t *ui_body_label(lv_obj_t *page);

/* Set label text only if it changed (avoids needless redraws) */
void ui_set_text(lv_obj_t *label, const char *text);

/* True once SNTP has set the system clock */
bool ui_time_synced(void);

#endif