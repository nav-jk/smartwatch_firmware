#include <string.h>
#include <time.h>

#include "ui_common.h"

void ui_make_inert(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color,
                   const char *text, lv_align_t align, int x, int y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_label_set_text(l, text);
    lv_obj_align(l, align, x, y);
    return l;
}

lv_obj_t *ui_shape(lv_obj_t *parent, int w, int h, int radius, lv_color_t col,
                   int dx, int dy)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_color(o, col, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    ui_make_inert(o);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    return o;
}

lv_obj_t *ui_outline(lv_obj_t *parent, int w, int h, int border, lv_color_t col,
                     int dx, int dy)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, border, 0);
    lv_obj_set_style_border_color(o, col, 0);
    lv_obj_set_style_border_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    ui_make_inert(o);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    return o;
}

lv_obj_t *ui_ring(lv_obj_t *parent, int size, int width, lv_color_t color,
                  lv_color_t track, int range_max)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, range_max);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    ui_make_inert(arc);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, track, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    return arc;
}

lv_obj_t *ui_pill(lv_obj_t *parent, int w, int h, int dx, int dy,
                  lv_color_t dot, const char *text)
{
    lv_obj_t *p = ui_shape(parent, w, h, LV_RADIUS_CIRCLE, COL_CARD, dx, dy);

    lv_obj_t *d = ui_shape(p, 8, 8, LV_RADIUS_CIRCLE, dot, 0, 0);
    lv_obj_align(d, LV_ALIGN_LEFT_MID, 10, 0);

    return ui_label(p, &lv_font_montserrat_14, COL_TEXT, text,
                    LV_ALIGN_LEFT_MID, 24, 0);
}

lv_obj_t *ui_body_label(lv_obj_t *page)
{
    lv_obj_t *l = ui_label(page, &lv_font_montserrat_20, COL_TEXT, "",
                           LV_ALIGN_CENTER, 0, 2);
    lv_obj_set_width(l, 200);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    return l;
}

void ui_set_text(lv_obj_t *label, const char *text)
{
    if (label == NULL) {
        return;
    }
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

bool ui_time_synced(void)
{
    return time(NULL) > 1704067200;   /* after 2024-01-01 */
}