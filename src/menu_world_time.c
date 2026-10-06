#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "menu.h"
#include "ui_common.h"

#define WT_TXT   0xFFFFFF
#define WT_DIM   0x8E8E93
#define WT_CARD  0x1C1C1E
#define WT_BG    0x000000
#define WT_ACC   0xFF9F0A      /* hero label accent (orange, like the reference) */

/* Layout (percent of page height) */
#define HERO_LBL_PCT   11
#define HERO_TIME_PCT  19
#define GRID_Y_PCT     48
#define CARD_H         44
#define CARD_GAP       4
#define EDGE_MARGIN    6
#define HERO_CITY      "KOLKATA"

/* Big time font: Montserrat 48 if enabled, else 28 */
#if defined(LV_FONT_MONTSERRAT_48) && LV_FONT_MONTSERRAT_48
#define FONT_HERO (&lv_font_montserrat_48)
#else
#define FONT_HERO (&lv_font_montserrat_28)
#endif

/* Zones without daylight saving, so the fixed offsets stay correct all year */
static const struct {
    const char *code;
    int offset;
    uint32_t color;
} zones[] = {
    { "UTC", 0,         0x0A84FF },
    { "DXB", 4 * 3600,  0xFF9F0A },
    { "SIN", 8 * 3600,  0x30D158 },
    { "TYO", 9 * 3600,  0xBF5AF2 },
};

#define ZONE_COUNT ((int)(sizeof(zones) / sizeof(zones[0])))

static lv_obj_t *body;
static lv_obj_t *hero_lbl;
static lv_obj_t *time_lbl[ZONE_COUNT];

static void world_time_icon(lv_obj_t *icon)
{
    ui_outline(icon, 30, 30, 3, COL_TEXT, 0, 0);
    ui_shape(icon, 26, 2, 0, COL_TEXT, 0, 0);
    ui_shape(icon, 2, 26, 0, COL_TEXT, 0, 0);
    ui_outline(icon, 14, 30, 2, COL_TEXT, 0, 0);
}

static void world_time_refresh(void)
{
    if (!body) return;

    if (!ui_time_synced()) {
        ui_set_text(hero_lbl, "--:--");
        for (int i = 0; i < ZONE_COUNT; i++) ui_set_text(time_lbl[i], "--:--");
        return;
    }

    time_t now = time(NULL);
    char buf[8];

    struct tm lt;
    localtime_r(&now, &lt);
    snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
    ui_set_text(hero_lbl, buf);

    for (int i = 0; i < ZONE_COUNT; i++) {
        time_t z = now + zones[i].offset;
        struct tm t;
        gmtime_r(&z, &t);
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        ui_set_text(time_lbl[i], buf);
    }
}

/* Width of the circle's chord at distance dy from the center */
static int chord_w(int R, int dy)
{
    if (dy >= R) return 0;
    return (int)(2.0f * sqrtf((float)(R * R - dy * dy)));
}

static void world_time_open(lv_obj_t *page)
{
    body = page;

    lv_obj_set_style_bg_color(page, lv_color_hex(WT_BG), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_update_layout(page);
    int W = lv_obj_get_width(page);
    int H = lv_obj_get_height(page);
    int R = (W < H ? W : H) / 2;

    /* Hero: city label + big local time */
    ui_label(page, &lv_font_montserrat_14, lv_color_hex(WT_ACC), HERO_CITY,
             LV_ALIGN_TOP_MID, 0, H * HERO_LBL_PCT / 100);
    hero_lbl = ui_label(page, FONT_HERO, lv_color_hex(WT_TXT), "--:--",
                        LV_ALIGN_TOP_MID, 0, H * HERO_TIME_PCT / 100);

    /* 2x2 grid, width limited by the circle at the lowest edge */
    int y0 = H * GRID_Y_PCT / 100;
    int y_end = y0 + CARD_H * 2 + CARD_GAP;
    int d0 = abs(y0 - H / 2);
    int d1 = abs(y_end - H / 2);
    int dy = d0 > d1 ? d0 : d1;

    int gw = chord_w(R, dy) - EDGE_MARGIN * 2;
    if (gw > W - 24) gw = W - 24;
    if (gw < 120) gw = 120;

    int cw = (gw - CARD_GAP) / 2;
    int x0 = (W - (cw * 2 + CARD_GAP)) / 2;

    for (int i = 0; i < ZONE_COUNT; i++) {
        int col = i % 2, row = i / 2;

        lv_obj_t *card = lv_obj_create(page);
        lv_obj_remove_style_all(card);
        lv_obj_set_size(card, cw, CARD_H);
        lv_obj_set_pos(card, x0 + col * (cw + CARD_GAP),
                       y0 + row * (CARD_H + CARD_GAP));
        lv_obj_set_style_radius(card, 14, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(WT_CARD), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        ui_label(card, &lv_font_montserrat_14, lv_color_hex(zones[i].color),
                 zones[i].code, LV_ALIGN_TOP_LEFT, 10, 3);
        time_lbl[i] = ui_label(card, &lv_font_montserrat_20,
                               lv_color_hex(WT_TXT), "--:--",
                               LV_ALIGN_BOTTOM_LEFT, 10, -3);
    }

    world_time_refresh();
}

static void world_time_close(void)
{
    body = NULL;
    hero_lbl = NULL;
    for (int i = 0; i < ZONE_COUNT; i++) time_lbl[i] = NULL;
}

const menu_app_t menu_world_time_app = {
    .title = "",
    .color = 0x0A84FF,
    .build_icon = world_time_icon,
    .open = world_time_open,
    .refresh = world_time_refresh,
    .close = world_time_close,
};