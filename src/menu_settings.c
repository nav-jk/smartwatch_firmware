#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_wifi.h"

#include "menu.h"
#include "ota.h"
#include "ui_common.h"

#define ST_TXT    0xFFFFFF
#define ST_DIM    0x8E8E93
#define ST_PILL   0x1C1C1E
#define ST_OFF    0x2C2C2E
#define ST_BG     0x000000

#define TILES_Y_PCT   12
#define PILLS_Y_PCT   46
#define PILL_H        30
#define PILL_GAP      4
#define TILE_GAP      8
#define EDGE_MARGIN   6

#define TILE_COUNT 3

static const struct {
    const char *symbol;
    uint32_t on_color;
} tiles[TILE_COUNT] = {
    { LV_SYMBOL_WIFI,     0x0A84FF },
    { LV_SYMBOL_MUTE,     0xFF9F0A },
    { LV_SYMBOL_EYE_OPEN, 0x30D158 },
};

static bool tile_on[TILE_COUNT] = {
    false,
    false,
    false
};

static lv_obj_t *body;
static lv_obj_t *tile_obj[TILE_COUNT];
static lv_obj_t *tile_ico[TILE_COUNT];
static lv_obj_t *ssid_lbl;
static lv_obj_t *rssi_lbl;

static void settings_icon(lv_obj_t *icon)
{
    ui_label(
        icon,
        &lv_font_montserrat_28,
        COL_TEXT,
        LV_SYMBOL_SETTINGS,
        LV_ALIGN_CENTER,
        0,
        0
    );
}

static void tile_style(int i)
{
    if (!tile_obj[i]) {
        return;
    }

    bool on = tile_on[i];

    lv_obj_set_style_bg_color(
        tile_obj[i],
        lv_color_hex(
            on ? tiles[i].on_color : ST_OFF
        ),
        0
    );

    lv_obj_set_style_text_color(
        tile_ico[i],
        lv_color_hex(
            on ? ST_TXT : ST_DIM
        ),
        0
    );
}

static void tile_clicked(lv_event_t *e)
{
    int i = (int)(intptr_t)
        lv_event_get_user_data(e);

    tile_on[i] = !tile_on[i];

    tile_style(i);
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

        ui_set_text(
            ssid_lbl,
            (const char *)ap.ssid
        );

        snprintf(
            buf,
            sizeof(buf),
            "%d dBm",
            ap.rssi
        );

        ui_set_text(
            rssi_lbl,
            buf
        );
    } else {
        tile_on[0] = false;

        ui_set_text(
            ssid_lbl,
            "Offline"
        );

        ui_set_text(
            rssi_lbl,
            "--"
        );
    }

    tile_style(0);
}

static int chord_w(int R, int dy)
{
    if (dy >= R) {
        return 0;
    }

    return (int)(
        2.0f *
        sqrtf(
            (float)(R * R - dy * dy)
        )
    );
}

static int band_w(
    int R,
    int H,
    int W,
    int y0,
    int y1,
    int min_w
)
{
    int d0 = abs(y0 - H / 2);
    int d1 = abs(y1 - H / 2);
    int dy = d0 > d1 ? d0 : d1;

    int w =
        chord_w(R, dy) -
        EDGE_MARGIN * 2;

    if (w > W - 24) {
        w = W - 24;
    }

    if (w < min_w) {
        w = min_w;
    }

    return w;
}

static lv_obj_t *info_pill(
    lv_obj_t *page,
    int x,
    int y,
    int w,
    const char *caption,
    const char *value
)
{
    lv_obj_t *p = lv_obj_create(page);

    lv_obj_remove_style_all(p);

    lv_obj_set_size(
        p,
        w,
        PILL_H
    );

    lv_obj_set_pos(
        p,
        x,
        y
    );

    lv_obj_set_style_radius(
        p,
        PILL_H / 2,
        0
    );

    lv_obj_set_style_bg_color(
        p,
        lv_color_hex(ST_PILL),
        0
    );

    lv_obj_set_style_bg_opa(
        p,
        LV_OPA_COVER,
        0
    );

    lv_obj_clear_flag(
        p,
        LV_OBJ_FLAG_SCROLLABLE
    );

    ui_label(
        p,
        &lv_font_montserrat_14,
        lv_color_hex(ST_DIM),
        caption,
        LV_ALIGN_LEFT_MID,
        14,
        0
    );

    lv_obj_t *v = ui_label(
        p,
        &lv_font_montserrat_14,
        lv_color_hex(ST_TXT),
        value,
        LV_ALIGN_RIGHT_MID,
        -14,
        0
    );

    lv_obj_set_width(
        v,
        w - 80
    );

    lv_label_set_long_mode(
        v,
        LV_LABEL_LONG_DOT
    );

    lv_obj_set_style_text_align(
        v,
        LV_TEXT_ALIGN_RIGHT,
        0
    );

    return v;
}

static void settings_open(lv_obj_t *page)
{
    body = page;

    lv_obj_set_style_bg_color(
        page,
        lv_color_hex(ST_BG),
        0
    );

    lv_obj_set_style_bg_opa(
        page,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_scrollbar_mode(
        page,
        LV_SCROLLBAR_MODE_OFF
    );

    lv_obj_update_layout(page);

    int W = lv_obj_get_width(page);
    int H = lv_obj_get_height(page);
    int R = (W < H ? W : H) / 2;

    int ty =
        H * TILES_Y_PCT / 100;

    int row_w =
        band_w(
            R,
            H,
            W,
            ty,
            ty + 48,
            120
        );

    int d =
        (row_w -
         TILE_GAP * (TILE_COUNT - 1)) /
        TILE_COUNT;

    if (d > 50) {
        d = 50;
    }

    int tx =
        (W -
         (d * TILE_COUNT +
          TILE_GAP * (TILE_COUNT - 1))) /
        2;

    for (int i = 0; i < TILE_COUNT; i++) {
        lv_obj_t *t = lv_obj_create(page);

        lv_obj_remove_style_all(t);

        lv_obj_set_size(
            t,
            d,
            d
        );

        lv_obj_set_pos(
            t,
            tx + i * (d + TILE_GAP),
            ty
        );

        lv_obj_set_style_radius(
            t,
            LV_RADIUS_CIRCLE,
            0
        );

        lv_obj_set_style_bg_opa(
            t,
            LV_OPA_COVER,
            0
        );

        lv_obj_clear_flag(
            t,
            LV_OBJ_FLAG_SCROLLABLE
        );

        tile_ico[i] = ui_label(
            t,
            &lv_font_montserrat_20,
            lv_color_hex(ST_DIM),
            tiles[i].symbol,
            LV_ALIGN_CENTER,
            0,
            0
        );

        tile_obj[i] = t;

        if (i > 0) {
            lv_obj_add_flag(
                t,
                LV_OBJ_FLAG_CLICKABLE
            );

            lv_obj_add_event_cb(
                t,
                tile_clicked,
                LV_EVENT_CLICKED,
                (void *)(intptr_t)i
            );
        }

        tile_style(i);
    }

    int py =
        H * PILLS_Y_PCT / 100;

    int pend =
        py +
        PILL_H * 3 +
        PILL_GAP * 2;

    int pw =
        band_w(
            R,
            H,
            W,
            py,
            pend,
            130
        );

    int px =
        (W - pw) / 2;

    ssid_lbl = info_pill(
        page,
        px,
        py,
        pw,
        "Wi-Fi",
        "--"
    );

    rssi_lbl = info_pill(
        page,
        px,
        py + PILL_H + PILL_GAP,
        pw,
        "Signal",
        "--"
    );

    info_pill(
        page,
        px,
        py + (PILL_H + PILL_GAP) * 2,
        pw,
        "Zone",
        "IST +5:30"
    );

    ota_bind_ui(page);

    settings_refresh();
}

static void settings_close(void)
{
    ota_unbind_ui();

    body = NULL;

    ssid_lbl = NULL;
    rssi_lbl = NULL;

    for (int i = 0; i < TILE_COUNT; i++) {
        tile_obj[i] = NULL;
        tile_ico[i] = NULL;
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