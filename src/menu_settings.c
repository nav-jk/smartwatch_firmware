#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_wifi.h"

#include "menu.h"
#include "ota.h"
#include "ui_common.h"
#include "esp_lvgl_port.h"
#include "settings_update.h"

#define ST_TXT    0xFFFFFF
#define ST_DIM    0x8E8E93
#define ST_PILL   0x1C1C1E
#define ST_OFF    0x2C2C2E
#define ST_SEP    0x2A2A2D
#define ST_BG     0x000000

#define TILES_Y_PCT   16
#define CARD_Y_PCT    42
#define ROW_H         32
#define ROW_COUNT     3
#define CARD_RADIUS   20
#define TILE_GAP      10
#define EDGE_MARGIN   6

#define TILE_COUNT 3
#define BAR_COUNT  4

static const struct {
    const char *symbol;
    uint32_t on_color;
} tiles[TILE_COUNT] = {
    { LV_SYMBOL_WIFI,     0x0A84FF },
    { LV_SYMBOL_MUTE,     0xFF9F0A },
    { LV_SYMBOL_EYE_OPEN, 0x30D158 },
};

static bool tile_on[TILE_COUNT] = { false, false, false };

static lv_obj_t *body;
static lv_obj_t *tile_obj[TILE_COUNT];
static lv_obj_t *tile_ico[TILE_COUNT];
static lv_obj_t *ssid_lbl;
static lv_obj_t *rssi_lbl;
static lv_obj_t *rssi_bar[BAR_COUNT];

static void settings_icon(lv_obj_t *icon)
{
    ui_label(icon, &lv_font_montserrat_28, COL_TEXT, LV_SYMBOL_SETTINGS,
             LV_ALIGN_CENTER, 0, 0);
}

/* Filled circle when on, dark circle with dim glyph when off. */
static void tile_style(int i)
{
    if (!tile_obj[i]) {
        return;
    }

    bool on = tile_on[i];

    lv_obj_set_style_bg_color(tile_obj[i],
                              lv_color_hex(on ? tiles[i].on_color : ST_OFF), 0);

    /* soft light rim when active, no rim when off */
    lv_obj_set_style_border_width(tile_obj[i], on ? 2 : 0, 0);
    lv_obj_set_style_border_color(tile_obj[i], lv_color_hex(ST_TXT), 0);
    lv_obj_set_style_border_opa(tile_obj[i], LV_OPA_40, 0);

    lv_obj_set_style_text_color(tile_ico[i],
                                lv_color_hex(on ? ST_TXT : ST_DIM), 0);
}

static void tile_clicked(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);

    tile_on[i] = !tile_on[i];
    tile_style(i);
}

/* 0..4 filled bars from RSSI; -1 means offline */
static int rssi_to_bars(int rssi)
{
    if (rssi >= -55) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -75) return 2;
    if (rssi >= -85) return 1;
    return 0;
}

static void bars_set(int filled)
{
    for (int i = 0; i < BAR_COUNT; i++) {
        if (!rssi_bar[i]) {
            continue;
        }

        lv_obj_set_style_bg_color(
            rssi_bar[i],
            lv_color_hex(i < filled ? ST_TXT : ST_OFF), 0);
    }
}

static void settings_refresh(void)
{
    if (!body) {
        return;
    }

    char buf[24];
    wifi_ap_record_t ap;

    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        tile_on[0] = true;

        ui_set_text(ssid_lbl, (const char *)ap.ssid);

        snprintf(buf, sizeof(buf), "%d dBm", ap.rssi);
        ui_set_text(rssi_lbl, buf);

        bars_set(rssi_to_bars(ap.rssi));
    } else {
        tile_on[0] = false;

        ui_set_text(ssid_lbl, "Offline");
        ui_set_text(rssi_lbl, "--");

        bars_set(0);
    }

    tile_style(0);
}

static int chord_w(int R, int dy)
{
    if (dy >= R) {
        return 0;
    }

    return (int)(2.0f * sqrtf((float)(R * R - dy * dy)));
}

static int band_w(int R, int H, int W, int y0, int y1, int min_w)
{
    int d0 = abs(y0 - H / 2);
    int d1 = abs(y1 - H / 2);
    int dy = d0 > d1 ? d0 : d1;

    int w = chord_w(R, dy) - EDGE_MARGIN * 2;

    if (w > W - 24) {
        w = W - 24;
    }

    if (w < min_w) {
        w = min_w;
    }

    return w;
}

static void make_inert(lv_obj_t *o)
{
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

/*
 * One row inside the grouped info card: dim caption on the left, white
 * value on the right, hairline separator above every row except the first.
 */
static lv_obj_t *info_row(lv_obj_t *card, int idx, int w,
                          const char *caption, const char *value)
{
    lv_obj_t *row = lv_obj_create(card);

    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, w, ROW_H);
    lv_obj_set_pos(row, 0, idx * ROW_H);
    make_inert(row);

    if (idx > 0) {
        lv_obj_t *sep = lv_obj_create(card);

        lv_obj_remove_style_all(sep);
        lv_obj_set_size(sep, w - 32, 1);
        lv_obj_set_pos(sep, 16, idx * ROW_H);
        lv_obj_set_style_bg_color(sep, lv_color_hex(ST_SEP), 0);
        lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
        make_inert(sep);
    }

    ui_label(row, &lv_font_montserrat_14, lv_color_hex(ST_DIM), caption,
             LV_ALIGN_LEFT_MID, 16, 0);

    lv_obj_t *v = ui_label(row, &lv_font_montserrat_14, lv_color_hex(ST_TXT),
                           value, LV_ALIGN_RIGHT_MID, -16, 0);

    lv_obj_set_width(v, w - 80);
    lv_label_set_long_mode(v, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_RIGHT, 0);

    return v;
}

/* Four ascending signal bars, placed on the Signal row between caption and value. */
static void build_signal_bars(lv_obj_t *card, int row_idx)
{
    static const int heights[BAR_COUNT] = { 4, 7, 10, 13 };

    int x = 62;
    int base = row_idx * ROW_H + ROW_H / 2 + 6;

    for (int i = 0; i < BAR_COUNT; i++) {
        lv_obj_t *b = lv_obj_create(card);

        lv_obj_remove_style_all(b);
        lv_obj_set_size(b, 3, heights[i]);
        lv_obj_set_pos(b, x + i * 5, base - heights[i]);
        lv_obj_set_style_radius(b, 1, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(ST_OFF), 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        make_inert(b);

        rssi_bar[i] = b;
    }
}

static void build_tile(lv_obj_t *page, int i, int x, int y, int d)
{
    lv_obj_t *t = lv_obj_create(page);

    lv_obj_remove_style_all(t);
    lv_obj_set_size(t, d, d);
    lv_obj_set_pos(t, x, y);

    lv_obj_set_style_radius(t, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE);

    tile_ico[i] = ui_label(t, &lv_font_montserrat_20, lv_color_hex(ST_DIM),
                           tiles[i].symbol, LV_ALIGN_CENTER, 0, 0);
    tile_obj[i] = t;

    /* Wi-Fi tile is a status indicator only; the other two toggle. */
    if (i > 0) {
        lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(t, tile_clicked, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        /* tactile press feedback: shrink slightly while held */
        lv_obj_set_style_transform_pivot_x(t, d / 2, 0);
        lv_obj_set_style_transform_pivot_y(t, d / 2, 0);
        lv_obj_set_style_transform_scale(t, 232, LV_STATE_PRESSED);
    }

    tile_style(i);
}

/* ------------------------------------------------------------------ */
/* Update screen: full-screen cloud UI shown while checking/updating   */
/* ------------------------------------------------------------------ */
#define UPD_BLUE   0x0A84FF
#define UPD_GREEN  0x30D158
#define UPD_RED    0xFF453A
#define UPD_CLOUD  0xEAF2FF

static lv_obj_t *upd_ov;
static lv_obj_t *upd_arc;
static lv_obj_t *upd_cloud;
static lv_obj_t *upd_icon;
static lv_obj_t *upd_title;
static lv_obj_t *upd_detail;
static lv_timer_t *upd_timer;

static void upd_bob_cb(void *obj, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t *)obj, v, 0);
}

static void upd_rot_cb(void *obj, int32_t v)
{
    lv_arc_set_rotation((lv_obj_t *)obj, (int)v);
}

static lv_obj_t *upd_shape(lv_obj_t *parent, int w, int h, int r,
                           lv_color_t col, int dx, int dy)
{
    lv_obj_t *o = lv_obj_create(parent);

    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, r, 0);
    lv_obj_set_style_bg_color(o, col, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    make_inert(o);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    return o;
}

static void upd_destroy(void)
{
    if (upd_timer) {
        lv_timer_delete(upd_timer);
        upd_timer = NULL;
    }

    if (upd_ov) {
        lv_anim_delete(upd_cloud, upd_bob_cb);
        lv_anim_delete(upd_arc, upd_rot_cb);
        lv_obj_delete(upd_ov);
    }

    upd_ov = NULL;
    upd_arc = NULL;
    upd_cloud = NULL;
    upd_icon = NULL;
    upd_title = NULL;
    upd_detail = NULL;
}

static void upd_set_indeterminate(void)
{
    lv_anim_delete(upd_arc, upd_rot_cb);

    lv_arc_set_value(upd_arc, 25);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, upd_arc);
    lv_anim_set_exec_cb(&a, upd_rot_cb);
    lv_anim_set_values(&a, 0, 360);
    lv_anim_set_duration(&a, 1100);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}

static void upd_set_color(uint32_t c)
{
    lv_obj_set_style_arc_color(upd_arc, lv_color_hex(c), LV_PART_INDICATOR);
    lv_obj_set_style_text_color(upd_icon, lv_color_hex(c), 0);
}

static void upd_build(void)
{
    upd_ov = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(upd_ov);
    lv_obj_set_size(upd_ov, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(upd_ov, lv_color_hex(ST_BG), 0);
    lv_obj_set_style_bg_opa(upd_ov, LV_OPA_COVER, 0);
    lv_obj_remove_flag(upd_ov, LV_OBJ_FLAG_SCROLLABLE);

    /* progress / spinner ring hugging the bezel */
    upd_arc = lv_arc_create(upd_ov);
    lv_obj_set_size(upd_arc, 230, 230);
    lv_obj_center(upd_arc);
    lv_arc_set_rotation(upd_arc, 270);
    lv_arc_set_bg_angles(upd_arc, 0, 360);
    lv_arc_set_range(upd_arc, 0, 100);
    lv_arc_set_value(upd_arc, 0);
    lv_obj_remove_style(upd_arc, NULL, LV_PART_KNOB);
    make_inert(upd_arc);
    lv_obj_set_style_arc_width(upd_arc, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(upd_arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(upd_arc, lv_color_hex(0x17191D), LV_PART_MAIN);
    lv_obj_set_style_arc_color(upd_arc, lv_color_hex(UPD_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(upd_arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(upd_arc, true, LV_PART_INDICATOR);

    /* cloud = base capsule + two puffs, with an icon on it */
    upd_cloud = lv_obj_create(upd_ov);
    lv_obj_remove_style_all(upd_cloud);
    lv_obj_set_size(upd_cloud, 120, 80);
    lv_obj_align(upd_cloud, LV_ALIGN_CENTER, 0, -40);
    make_inert(upd_cloud);

    lv_color_t c = lv_color_hex(UPD_CLOUD);

    upd_shape(upd_cloud, 46, 46, LV_RADIUS_CIRCLE, c, -22, -4);
    upd_shape(upd_cloud, 58, 58, LV_RADIUS_CIRCLE, c, 12, -12);
    upd_shape(upd_cloud, 104, 42, 21, c, 0, 12);

    upd_icon = ui_label(upd_cloud, &lv_font_montserrat_20,
                        lv_color_hex(UPD_BLUE), LV_SYMBOL_DOWNLOAD,
                        LV_ALIGN_CENTER, 0, 8);

    /* gentle floating motion */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, upd_cloud);
    lv_anim_set_exec_cb(&a, upd_bob_cb);
    lv_anim_set_values(&a, -4, 4);
    lv_anim_set_duration(&a, 1100);
    lv_anim_set_reverse_duration(&a, 1100);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    upd_title = ui_label(upd_ov, &lv_font_montserrat_20, lv_color_hex(ST_TXT),
                         "Checking...", LV_ALIGN_CENTER, 0, 38);

    upd_detail = ui_label(upd_ov, &lv_font_montserrat_14, lv_color_hex(ST_DIM),
                          "Please wait", LV_ALIGN_CENTER, 0, 66);
    lv_obj_set_width(upd_detail, 150);
    lv_label_set_long_mode(upd_detail, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(upd_detail, LV_TEXT_ALIGN_CENTER, 0);

    upd_set_indeterminate();
}

static void upd_close_cb(lv_timer_t *t)
{
    (void)t;

    upd_timer = NULL;   /* one-shot timer deletes itself */
    upd_destroy();
    settings_refresh();
}

void settings_update_open(const char *title)
{
    bool locked = lvgl_port_lock(0);

    if (body) {
        if (!upd_ov) {
            upd_build();
        } else {
            if (upd_timer) {
                lv_timer_delete(upd_timer);
                upd_timer = NULL;
            }
            upd_set_color(UPD_BLUE);
            lv_label_set_text(upd_icon, LV_SYMBOL_DOWNLOAD);
            upd_set_indeterminate();
        }

        lv_label_set_text(upd_title, title ? title : "Checking...");
        lv_label_set_text(upd_detail, "Please wait");
    }

    if (locked) {
        lvgl_port_unlock();
    }
}

void settings_update_status(const char *title, const char *detail)
{
    bool locked = lvgl_port_lock(0);

    if (upd_ov) {
        if (title) {
            lv_label_set_text(upd_title, title);
        }
        if (detail) {
            lv_label_set_text(upd_detail, detail);
        }
    }

    if (locked) {
        lvgl_port_unlock();
    }
}

void settings_update_progress(int pct)
{
    bool locked = lvgl_port_lock(0);

    if (upd_ov) {
        if (pct < 0) {
            upd_set_indeterminate();
        } else {
            if (pct > 100) {
                pct = 100;
            }
            lv_anim_delete(upd_arc, upd_rot_cb);
            lv_arc_set_rotation(upd_arc, 270);
            lv_arc_set_value(upd_arc, pct);
        }
    }

    if (locked) {
        lvgl_port_unlock();
    }
}

void settings_update_done(bool ok, const char *msg)
{
    bool locked = lvgl_port_lock(0);

    if (upd_ov) {
        lv_anim_delete(upd_arc, upd_rot_cb);
        lv_arc_set_rotation(upd_arc, 270);
        lv_arc_set_value(upd_arc, 100);

        upd_set_color(ok ? UPD_GREEN : UPD_RED);
        lv_label_set_text(upd_icon, ok ? LV_SYMBOL_OK : LV_SYMBOL_WARNING);

        lv_label_set_text(upd_title, ok ? "All set" : "Failed");
        lv_label_set_text(upd_detail, msg ? msg : "");

        /* show result briefly, then drop back to Settings */
        if (upd_timer) {
            lv_timer_delete(upd_timer);
        }
        upd_timer = lv_timer_create(upd_close_cb, 1800, NULL);
        lv_timer_set_repeat_count(upd_timer, 1);
    }

    if (locked) {
        lvgl_port_unlock();
    }
}

static void settings_open(lv_obj_t *page)
{
    body = page;

    lv_obj_set_style_bg_color(page, lv_color_hex(ST_BG), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_update_layout(page);

    int W = lv_obj_get_width(page);
    int H = lv_obj_get_height(page);
    int R = (W < H ? W : H) / 2;

    /* ---- quick toggle tiles ---- */
    int ty = H * TILES_Y_PCT / 100;

    int row_w = band_w(R, H, W, ty, ty + 48, 120);

    int d = (row_w - TILE_GAP * (TILE_COUNT - 1)) / TILE_COUNT;

    if (d > 50) {
        d = 50;
    }

    int tx = (W - (d * TILE_COUNT + TILE_GAP * (TILE_COUNT - 1))) / 2;

    for (int i = 0; i < TILE_COUNT; i++) {
        build_tile(page, i, tx + i * (d + TILE_GAP), ty, d);
    }

    /* ---- grouped info card ---- */
    int cy = H * CARD_Y_PCT / 100;
    int ch = ROW_H * ROW_COUNT;

    int cw = band_w(R, H, W, cy, cy + ch, 130);
    int cx = (W - cw) / 2;

    lv_obj_t *card = lv_obj_create(page);

    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, cw, ch);
    lv_obj_set_pos(card, cx, cy);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(ST_PILL), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    make_inert(card);

    ssid_lbl = info_row(card, 0, cw, "Wi-Fi", "--");
    rssi_lbl = info_row(card, 1, cw, "Signal", "--");
    info_row(card, 2, cw, "Zone", "IST +5:30");

    build_signal_bars(card, 1);

    ota_bind_ui(page);

    settings_refresh();
}

static void settings_close(void)
{
    upd_destroy();

    ota_unbind_ui();

    body = NULL;

    ssid_lbl = NULL;
    rssi_lbl = NULL;

    for (int i = 0; i < TILE_COUNT; i++) {
        tile_obj[i] = NULL;
        tile_ico[i] = NULL;
    }

    for (int i = 0; i < BAR_COUNT; i++) {
        rssi_bar[i] = NULL;
    }
}

const menu_app_t menu_settings_app = {
    .title = "Settings",
    .color = 0x8E8E93,
    .fullscreen = false,
    .build_icon = settings_icon,
    .open = settings_open,
    .refresh = settings_refresh,
    .close = settings_close,
};