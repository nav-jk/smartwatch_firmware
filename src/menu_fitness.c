#include <stdio.h>

#include "menu.h"
#include "mock_data.h"
#include "ui_common.h"

/* Layout tuned for a 240x240 round GC9A01 panel. */
#define FIT_GOAL_KCAL   6000
#define FIT_LIME        0xB6FF3B
#define FIT_BG_TOP      0x000000
#define FIT_BG_BOTTOM   0x0F9C8A

/* Use lv_font_montserrat_40 (or any large font) - enable it in menuconfig:
 * Component config -> LVGL -> Font usage -> Enable Montserrat 40 */
#define FIT_BIG_FONT    (&lv_font_montserrat_40)
#define FIT_SMALL_FONT  (&lv_font_montserrat_14)

static void fitness_icon(lv_obj_t *icon)
{
    ui_outline(icon, 30, 30, 4, COL_TEXT, 0, 0);
    ui_outline(icon, 14, 14, 4, lv_color_hex(0xFFD0D8), 0, 0);
}

/* Small flame-like badge: lime blob with a dark "eye" cut-out. */
static void fitness_flame(lv_obj_t *parent)
{
    lv_obj_t *flame = lv_obj_create(parent);
    lv_obj_remove_style_all(flame);
    lv_obj_set_size(flame, 26, 26);
    lv_obj_set_style_radius(flame, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(flame, lv_color_hex(FIT_LIME), 0);
    lv_obj_set_style_bg_opa(flame, LV_OPA_COVER, 0);
    lv_obj_align(flame, LV_ALIGN_TOP_MID, 0, 38);

    lv_obj_t *tip = lv_obj_create(flame);
    lv_obj_remove_style_all(tip);
    lv_obj_set_size(tip, 10, 10);
    lv_obj_set_style_radius(tip, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tip, lv_color_hex(FIT_LIME), 0);
    lv_obj_set_style_bg_opa(tip, LV_OPA_COVER, 0);
    lv_obj_align(tip, LV_ALIGN_TOP_MID, 0, -6);

    lv_obj_t *hole = lv_obj_create(flame);
    lv_obj_remove_style_all(hole);
    lv_obj_set_size(hole, 10, 10);
    lv_obj_set_style_radius(hole, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hole, lv_color_hex(0x0B3B34), 0);
    lv_obj_set_style_bg_opa(hole, LV_OPA_COVER, 0);
    lv_obj_align(hole, LV_ALIGN_CENTER, 0, 3);
}

static void fitness_open(lv_obj_t *page)
{
    char buf[16];

    /* Dark -> teal vertical gradient background */
    lv_obj_set_style_bg_color(page, lv_color_hex(FIT_BG_TOP), 0);
    lv_obj_set_style_bg_grad_color(page, lv_color_hex(FIT_BG_BOTTOM), 0);
    lv_obj_set_style_bg_grad_dir(page, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_stop(page, 60, 0);
    lv_obj_set_style_bg_grad_stop(page, 255, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);

    fitness_flame(page);

    /* "Burned Cal" caption */
    lv_obj_t *cap = lv_label_create(page);
    lv_label_set_text(cap, "Burned Cal");
    lv_obj_set_style_text_font(cap, FIT_SMALL_FONT, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(0xE6EEEC), 0);
    lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 74);

    /* Big number with thousands separator */
    snprintf(buf, sizeof(buf), "%d,%03d", MOCK_KCAL / 1000, MOCK_KCAL % 1000);
    if (MOCK_KCAL < 1000) {
        snprintf(buf, sizeof(buf), "%d", MOCK_KCAL);
    }
    lv_obj_t *val = lv_label_create(page);
    lv_label_set_text(val, buf);
    lv_obj_set_style_text_font(val, FIT_BIG_FONT, 0);
    lv_obj_set_style_text_color(val, lv_color_white(), 0);
    lv_obj_align(val, LV_ALIGN_TOP_MID, 0, 94);

    /* Bottom progress arc with round knob (fills left -> right) */
    int pct = MOCK_KCAL * 100 / FIT_GOAL_KCAL;
    if (pct > 100) pct = 100;
    if (pct < 0)   pct = 0;

    lv_obj_t *arc = lv_arc_create(page);
    lv_obj_set_size(arc, 224, 224);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_bg_angles(arc, 30, 150);          /* bottom sweep */
    lv_arc_set_mode(arc, LV_ARC_MODE_REVERSE);   /* grow from left end */
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, pct);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    /* track */
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x0A4F47), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    /* indicator */
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(FIT_LIME), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    /* knob: lime dot with a soft teal ring */
    lv_obj_set_style_bg_color(arc, lv_color_hex(0xD8FF9A), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 4, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc, 3, LV_PART_KNOB);
    lv_obj_set_style_border_color(arc, lv_color_hex(0x2FB59F), LV_PART_KNOB);
}

const menu_app_t menu_fitness_app = {
    .title = "Fitness",
    .color = 0xFF2D55,
    .build_icon = fitness_icon,
    .open = fitness_open,
};