#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "driver/gpio.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "specs.h"
#include "ui.h"

#define WIFI_SSID   "iPhone"
#define WIFI_PASS   "12345678"
#define TZ_STRING   "IST-5:30"

#define MOCK_STEPS      4820
#define MOCK_BPM        72
#define MOCK_KCAL       412
#define MOCK_MOVE_PCT   78
#define MOCK_EXERCISE_PCT 54
#define MOCK_STAND_PCT  91

/* ---- Palette (HyperOS-inspired: pure black, warm orange accent) ---- */
#define COL_BG        lv_color_hex(0x000000)
#define COL_TEXT      lv_color_hex(0xFFFFFF)
#define COL_MUTED     lv_color_hex(0x8E939B)
#define COL_ACCENT    lv_color_hex(0xFF7A3D)   /* warm orange */
#define COL_TRACK     lv_color_hex(0x17191D)
#define COL_SECOND    lv_color_hex(0xFF7A3D)
#define COL_DARK      lv_color_hex(0x1C1C1E)
#define COL_CARD      lv_color_hex(0x16181C)   /* pill / complication bg */
#define COL_TICK      lv_color_hex(0x3A3F47)
#define COL_GREEN     lv_color_hex(0x30D158)
#define COL_PINK      lv_color_hex(0xFF375F)

enum { PAGE_DIGITAL, PAGE_ANALOG, PAGE_ACTIVITY, PAGE_APPS, NUM_PAGES };

typedef enum {
    MENU_FITNESS,
    MENU_PHONE,
    MENU_INTERNATIONAL_TIME,
    MENU_WALLET,
    MENU_CALENDAR,
    MENU_WEATHER,
    MENU_WORKOUT,
    MENU_SETTINGS,
    MENU_COUNT
} menu_item_t;

static const char *menu_titles[MENU_COUNT] = {
    "Fitness", "Phone", "World Time", "Wallet",
    "Calendar", "Weather", "Workout", "Settings"
};

static const uint32_t menu_colors[MENU_COUNT] = {
    0xFF2D55, 0x30D158, 0x0A84FF, 0xFF9F0A,
    0xF2F2F7, 0x32ADE6, 0xFFD60A, 0x8E8E93
};

static const int8_t menu_x[MENU_COUNT] = {
    -62, 0, 62, -31, 31, -62, 0, 62
};

static const int8_t menu_y[MENU_COUNT] = {
    -54, -54, -54, 0, 0, 54, 54, 54
};

#define ICON_BASE  52
#define ICON_FOCUS 64

static lv_obj_t *tileview;
static lv_obj_t *tiles[NUM_PAGES];

/* digital face */
static lv_obj_t *ring;
static lv_obj_t *hour_label;
static lv_obj_t *min_label;
static lv_obj_t *colon_top;
static lv_obj_t *colon_bot;
static lv_obj_t *date_label;
static lv_obj_t *steps_label;
static lv_obj_t *bpm_label;

/* analog face */
static lv_obj_t *scale;
static lv_obj_t *hour_hand;
static lv_obj_t *min_hand;
static lv_obj_t *sec_hand;
static lv_obj_t *analog_date;
static lv_point_precise_t hour_pts[2];
static lv_point_precise_t min_pts[2];
static lv_point_precise_t sec_pts[2];

static lv_obj_t *act_arc[3];
static const int act_target[3] = {
    MOCK_MOVE_PCT,
    MOCK_EXERCISE_PCT,
    MOCK_STAND_PCT
};

static lv_obj_t *menu_icons[MENU_COUNT];
static int menu_focus = 0;
static lv_obj_t *cal_dow;
static lv_obj_t *cal_day;

static lv_obj_t *app_overlay;
static lv_obj_t *app_title;
static lv_obj_t *app_accent;
static lv_obj_t *app_body;
static lv_obj_t *app_hint;
static int open_app = -1;

static void make_inert(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color,
                            const char *text, lv_align_t align, int x, int y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_label_set_text(l, text);
    lv_obj_align(l, align, x, y);
    return l;
}

static lv_obj_t *shape(lv_obj_t *parent, int w, int h, int radius, lv_color_t col, int dx, int dy)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_color(o, col, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    make_inert(o);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    return o;
}

static lv_obj_t *outline(lv_obj_t *parent, int w, int h, int border, lv_color_t col, int dx, int dy)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, border, 0);
    lv_obj_set_style_border_color(o, col, 0);
    lv_obj_set_style_border_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    make_inert(o);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    return o;
}

static lv_obj_t *make_ring(lv_obj_t *parent, int size, int width, lv_color_t color,
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
    make_inert(arc);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, track, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    return arc;
}

/*
 * Complication pill: rounded dark capsule with a coloured dot and a value.
 * Returns the value label so it can be updated later.
 */
static lv_obj_t *make_pill(lv_obj_t *parent, int w, int h, int dx, int dy,
                           lv_color_t dot, const char *text)
{
    lv_obj_t *p = shape(parent, w, h, LV_RADIUS_CIRCLE, COL_CARD, dx, dy);

    shape(p, 8, 8, LV_RADIUS_CIRCLE, dot, 0, 0);
    lv_obj_align(lv_obj_get_child(p, 0), LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *l = make_label(p, &lv_font_montserrat_14, COL_TEXT, text,
                             LV_ALIGN_LEFT_MID, 24, 0);
    return l;
}

/* ------------------------------------------------------------------ */
/* Digital face                                                        */
/*                                                                     */
/*            MON 05 OCT                                               */
/*              10 : 42          <- hours white, minutes orange        */
/*          (o 4820) (o 72)      <- complication pills                 */
/*   orange seconds ring around the edge                               */
/* ------------------------------------------------------------------ */
static void build_digital(lv_obj_t *parent)
{
    /* faint inner hairline ring for depth */
    outline(parent, 204, 204, 1, lv_color_hex(0x101215), 0, 0);

    /* seconds ring: slim, rounded, warm accent */
    ring = make_ring(parent, 226, 6, COL_ACCENT, COL_TRACK, 59);

    date_label = make_label(parent, &lv_font_montserrat_14, COL_MUTED, "Syncing...",
                            LV_ALIGN_CENTER, 0, -62);

    /* hours | colon | minutes, each centred so digits never jitter */
    hour_label = make_label(parent, &lv_font_montserrat_48, COL_TEXT, "--",
                            LV_ALIGN_CENTER, -40, -8);
    min_label = make_label(parent, &lv_font_montserrat_48, COL_ACCENT, "--",
                           LV_ALIGN_CENTER, 40, -8);

    colon_top = shape(parent, 6, 6, LV_RADIUS_CIRCLE, COL_MUTED, 0, -16);
    colon_bot = shape(parent, 6, 6, LV_RADIUS_CIRCLE, COL_MUTED, 0, 2);

    char buf[16];

    snprintf(buf, sizeof(buf), "%d", MOCK_STEPS);
    steps_label = make_pill(parent, 76, 28, -40, 50, COL_GREEN, buf);

    snprintf(buf, sizeof(buf), "%d", MOCK_BPM);
    bpm_label = make_pill(parent, 76, 28, 40, 50, COL_PINK, buf);
}

#define ANALOG_RANGE 600

static lv_obj_t *make_hand(lv_obj_t *parent, lv_point_precise_t *pts, int width, lv_color_t color)
{
    lv_obj_t *hand = lv_line_create(parent);
    lv_line_set_points_mutable(hand, pts, 2);
    lv_obj_set_style_line_width(hand, width, 0);
    lv_obj_set_style_line_color(hand, color, 0);
    lv_obj_set_style_line_rounded(hand, true, 0);
    return hand;
}

/* ------------------------------------------------------------------ */
/* Analog face: minimal ticks, 12/3/6/9 numerals, date pill, slim hands */
/* ------------------------------------------------------------------ */
static void build_analog(lv_obj_t *parent)
{
    /* date pill under the 12 (created first so hands draw above it) */
    lv_obj_t *pill = shape(parent, 72, 24, LV_RADIUS_CIRCLE, COL_CARD, 0, -40);
    analog_date = make_label(pill, &lv_font_montserrat_14, COL_MUTED, "",
                             LV_ALIGN_CENTER, 0, 0);

    /* four numerals, drawn beneath the hands */
    make_label(parent, &lv_font_montserrat_20, COL_TEXT, "12", LV_ALIGN_CENTER, 0, -80);
    make_label(parent, &lv_font_montserrat_20, COL_TEXT, "3", LV_ALIGN_CENTER, 82, 0);
    make_label(parent, &lv_font_montserrat_20, COL_TEXT, "6", LV_ALIGN_CENTER, 0, 80);
    make_label(parent, &lv_font_montserrat_20, COL_TEXT, "9", LV_ALIGN_CENTER, -82, 0);

    scale = lv_scale_create(parent);
    lv_obj_set_size(scale, 226, 226);
    lv_obj_center(scale);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_obj_set_style_bg_opa(scale, LV_OPA_TRANSP, 0);     /* let numerals show through */
    lv_obj_set_style_border_width(scale, 0, 0);
    lv_obj_set_style_radius(scale, LV_RADIUS_CIRCLE, 0);

    lv_scale_set_label_show(scale, false);
    lv_scale_set_total_tick_count(scale, 61);
    lv_scale_set_major_tick_every(scale, 5);
    lv_scale_set_range(scale, 0, ANALOG_RANGE);
    lv_scale_set_angle_range(scale, 360);
    lv_scale_set_rotation(scale, 270);

    lv_obj_set_style_length(scale, 3, LV_PART_ITEMS);
    lv_obj_set_style_length(scale, 8, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 1, LV_PART_ITEMS);
    lv_obj_set_style_line_width(scale, 3, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale, COL_TICK, LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale, COL_MUTED, LV_PART_INDICATOR);

    hour_hand = make_hand(scale, hour_pts, 7, COL_TEXT);
    min_hand = make_hand(scale, min_pts, 4, lv_color_hex(0xD6D8DC));
    sec_hand = make_hand(scale, sec_pts, 2, COL_SECOND);

    lv_scale_set_line_needle_value(scale, hour_hand, 46, 0);
    lv_scale_set_line_needle_value(scale, min_hand, 70, 0);
    lv_scale_set_line_needle_value(scale, sec_hand, 86, 0);

    /* hub: orange disc with black centre */
    shape(parent, 14, 14, LV_RADIUS_CIRCLE, COL_SECOND, 0, 0);
    shape(parent, 5, 5, LV_RADIUS_CIRCLE, COL_BG, 0, 0);
}

static void build_activity(lv_obj_t *parent)
{
    act_arc[0] = make_ring(parent, 226, 18, lv_color_hex(0xFF2D55),
                            lv_color_hex(0x33101A), 100);
    act_arc[1] = make_ring(parent, 182, 18, lv_color_hex(0x92E82A),
                            lv_color_hex(0x1D2E0D), 100);
    act_arc[2] = make_ring(parent, 138, 18, lv_color_hex(0x00C7FF),
                            lv_color_hex(0x00252E), 100);

    char kcal[16];
    snprintf(kcal, sizeof(kcal), "%d", MOCK_KCAL);

    make_label(parent, &lv_font_montserrat_28, COL_TEXT, kcal,
               LV_ALIGN_CENTER, 0, -8);
    make_label(parent, &lv_font_montserrat_14, COL_MUTED, "kcal",
               LV_ALIGN_CENTER, 0, 18);
}

static void arc_anim_cb(void *obj, int32_t v)
{
    lv_arc_set_value((lv_obj_t *)obj, v);
}

static void play_activity_anim(void)
{
    for (int i = 0; i < 3; i++) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, act_arc[i]);
        lv_anim_set_exec_cb(&a, arc_anim_cb);
        lv_anim_set_values(&a, 0, act_target[i]);
        lv_anim_set_duration(&a, 900);
        lv_anim_set_delay(&a, i * 140);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
    }
}

static lv_obj_t *make_icon_base(lv_obj_t *parent, int idx)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, ICON_BASE, ICON_BASE);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(menu_colors[idx]), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_border_color(c, COL_TEXT, 0);
    lv_obj_set_style_border_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    make_inert(c);
    lv_obj_align(c, LV_ALIGN_CENTER, menu_x[idx], menu_y[idx]);
    return c;
}

static void build_apps(lv_obj_t *parent)
{
    lv_color_t white = COL_TEXT;

    for (int i = 0; i < MENU_COUNT; i++) {
        lv_obj_t *c = make_icon_base(parent, i);
        menu_icons[i] = c;

        switch (i) {
        case MENU_FITNESS:
            outline(c, 30, 30, 4, white, 0, 0);
            outline(c, 14, 14, 4, lv_color_hex(0xFFD0D8), 0, 0);
            break;

        case MENU_PHONE: {
            lv_obj_t *g = make_label(c, &lv_font_montserrat_28, white,
                                     LV_SYMBOL_CALL, LV_ALIGN_CENTER, 0, 0);
            (void)g;
            break;
        }

        case MENU_INTERNATIONAL_TIME:
            outline(c, 30, 30, 3, white, 0, 0);
            shape(c, 26, 2, 0, white, 0, 0);
            shape(c, 2, 26, 0, white, 0, 0);
            outline(c, 14, 30, 2, white, 0, 0);
            break;

        case MENU_WALLET:
            shape(c, 32, 22, 5, white, 0, 2);
            shape(c, 32, 8, 4, lv_color_hex(0xFFE0B2), 0, -6);
            shape(c, 7, 7, LV_RADIUS_CIRCLE,
                  lv_color_hex(menu_colors[i]), 10, 4);
            break;

        case MENU_CALENDAR:
            cal_dow = make_label(c, &lv_font_montserrat_14,
                                 lv_color_hex(0xFF3B30), "---",
                                 LV_ALIGN_CENTER, 0, -11);
            cal_day = make_label(c, &lv_font_montserrat_20,
                                 COL_DARK, "--",
                                 LV_ALIGN_CENTER, 0, 8);
            break;

        case MENU_WEATHER:
            shape(c, 16, 16, LV_RADIUS_CIRCLE,
                  lv_color_hex(0xFFD60A), -8, -8);
            shape(c, 30, 14, 7, white, 3, 8);
            shape(c, 16, 16, LV_RADIUS_CIRCLE, white, 4, 1);
            break;

        case MENU_WORKOUT:
            shape(c, 26, 4, 2, COL_DARK, 0, 0);
            shape(c, 6, 20, 3, COL_DARK, -13, 0);
            shape(c, 6, 20, 3, COL_DARK, 13, 0);
            shape(c, 4, 12, 2, COL_DARK, -19, 0);
            shape(c, 4, 12, 2, COL_DARK, 19, 0);
            break;

        case MENU_SETTINGS:
            make_label(c, &lv_font_montserrat_28, white,
                       LV_SYMBOL_SETTINGS, LV_ALIGN_CENTER, 0, 0);
            break;

        default:
            break;
        }
    }
}

static void icon_size_cb(void *obj, int32_t v)
{
    lv_obj_set_size((lv_obj_t *)obj, v, v);
}

static void animate_icon(lv_obj_t *obj, int from, int to)
{
    lv_anim_delete(obj, icon_size_cb);

    if (from == to) {
        lv_obj_set_size(obj, to, to);
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, icon_size_cb);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, 180);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void set_focus(int idx)
{
    lv_obj_t *old = menu_icons[menu_focus];

    if (menu_focus != idx) {
        lv_obj_set_style_border_width(old, 0, 0);
        animate_icon(old, lv_obj_get_width(old), ICON_BASE);
    }

    menu_focus = idx;

    lv_obj_t *cur = menu_icons[idx];
    lv_obj_move_foreground(cur);
    lv_obj_set_style_border_width(cur, 3, 0);
    animate_icon(cur, lv_obj_get_width(cur), ICON_FOCUS);
}

static void reset_focus(void)
{
    for (int i = 0; i < MENU_COUNT; i++) {
        lv_anim_delete(menu_icons[i], icon_size_cb);
        lv_obj_set_size(menu_icons[i], ICON_BASE, ICON_BASE);
        lv_obj_set_style_border_width(menu_icons[i], 0, 0);
    }

    menu_focus = 0;
    set_focus(0);
}

static void app_refresh_body(void)
{
    if (open_app < 0) {
        return;
    }

    time_t now;
    time(&now);

    struct tm tm;
    localtime_r(&now, &tm);

    bool synced = tm.tm_year >= (2024 - 1900);

    char buf[160];

    switch (open_app) {
    case MENU_FITNESS:
        snprintf(buf, sizeof(buf), "%d steps\n%d kcal\n%d bpm\n(demo data)",
                 MOCK_STEPS, MOCK_KCAL, MOCK_BPM);
        break;

    case MENU_PHONE:
        snprintf(buf, sizeof(buf), "Not connected\nPair with a phone");
        break;

    case MENU_INTERNATIONAL_TIME: {
        static const struct {
            const char *name;
            int offset;
        } zones[] = {
            { "UTC", 0 },
            { "Dubai", 4 * 3600 },
            { "Singapore", 8 * 3600 },
            { "Tokyo", 9 * 3600 },
        };

        if (!synced) {
            snprintf(buf, sizeof(buf), "Syncing...");
        } else {
            int n = 0;
            buf[0] = '\0';

            for (int i = 0; i < 4; i++) {
                time_t z = now + zones[i].offset;
                struct tm t;
                gmtime_r(&z, &t);

                n += snprintf(buf + n, sizeof(buf) - n,
                              "%s%s  %02d:%02d",
                              i ? "\n" : "",
                              zones[i].name,
                              t.tm_hour,
                              t.tm_min);
            }
        }
        break;
    }

    case MENU_WALLET:
        snprintf(buf, sizeof(buf), "No cards yet");
        break;

    case MENU_CALENDAR:
        if (synced) {
            strftime(buf, sizeof(buf), "%A\n%d %B\n\nNo events", &tm);
        } else {
            snprintf(buf, sizeof(buf), "Syncing...");
        }
        break;

    case MENU_WEATHER:
        snprintf(buf, sizeof(buf), "No data yet\nNeeds a weather API");
        break;

    case MENU_WORKOUT:
        snprintf(buf, sizeof(buf), "Ready\nComing soon");
        break;

    case MENU_SETTINGS:
        snprintf(buf, sizeof(buf),
                 "Wi-Fi: %s\nZone: IST (UTC+5:30)\nGC9A01 240x240",
                 WIFI_SSID);
        break;

    default:
        buf[0] = '\0';
        break;
    }

    lv_label_set_text(app_body, buf);
}

static void build_app_overlay(void)
{
    app_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(app_overlay, LCD_H_RES, LCD_V_RES);
    lv_obj_align(app_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(app_overlay, 0, 0);
    lv_obj_set_style_bg_color(app_overlay, COL_BG, 0);
    lv_obj_set_style_bg_opa(app_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(app_overlay, 0, 0);
    lv_obj_set_style_pad_all(app_overlay, 0, 0);
    make_inert(app_overlay);

    app_title = make_label(app_overlay, &lv_font_montserrat_20,
                            COL_TEXT, "", LV_ALIGN_CENTER, 0, -80);

    app_accent = shape(app_overlay, 36, 4, 2,
                       COL_ACCENT, 0, -58);

    app_body = make_label(app_overlay, &lv_font_montserrat_20,
                          COL_TEXT, "", LV_ALIGN_CENTER, 0, 2);
    lv_obj_set_width(app_body, 200);
    lv_obj_set_style_text_align(app_body, LV_TEXT_ALIGN_CENTER, 0);

    app_hint = make_label(app_overlay, &lv_font_montserrat_14,
                          COL_MUTED, "Press to go back",
                          LV_ALIGN_CENTER, 0, 88);

    lv_obj_add_flag(app_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void open_app_screen(int idx)
{
    open_app = idx;

    lv_label_set_text(app_title, menu_titles[idx]);
    lv_obj_set_style_bg_color(app_accent,
                               lv_color_hex(menu_colors[idx]), 0);

    app_refresh_body();
    lv_obj_remove_flag(app_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void close_app_screen(void)
{
    open_app = -1;
    lv_obj_add_flag(app_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void update_ui(lv_timer_t *t)
{
    (void)t;

    static const char *dow_names[7] = {
        "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
    };

    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t sec_now = tv.tv_sec;

    struct tm tm;
    localtime_r(&sec_now, &tm);

    bool synced = tm.tm_year >= (2024 - 1900);
    int ms = (int)(tv.tv_usec / 1000);

    if (!synced) {
        memset(&tm, 0, sizeof(tm));
        ms = 0;
    }

    static int last_min = -2;
    static int last_sec = -2;

    int min_key = synced ? tm.tm_min + tm.tm_hour * 60 : -1;

    if (min_key != last_min) {
        last_min = min_key;

        if (synced) {
            char date[24];
            char short_date[16];

            lv_label_set_text_fmt(hour_label, "%02d", tm.tm_hour);
            lv_label_set_text_fmt(min_label, "%02d", tm.tm_min);

            /* "MON 05 OCT" style header, uppercase like HyperOS */
            strftime(short_date, sizeof(short_date), "%b", &tm);
            for (char *p = short_date; *p; p++) {
                if (*p >= 'a' && *p <= 'z') {
                    *p -= 32;
                }
            }
            snprintf(date, sizeof(date), "%s %02d %s",
                     dow_names[tm.tm_wday % 7], tm.tm_mday, short_date);

            lv_label_set_text(date_label, date);

            snprintf(date, sizeof(date), "%s %02d",
                     dow_names[tm.tm_wday % 7], tm.tm_mday);
            lv_label_set_text(analog_date, date);

            lv_label_set_text(cal_dow, dow_names[tm.tm_wday % 7]);
            lv_label_set_text_fmt(cal_day, "%d", tm.tm_mday);
        } else {
            lv_label_set_text(hour_label, "--");
            lv_label_set_text(min_label, "--");
            lv_label_set_text(date_label, "Syncing...");
            lv_label_set_text(analog_date, "--");
            lv_label_set_text(cal_dow, "---");
            lv_label_set_text(cal_day, "--");
        }

        app_refresh_body();
    }

    if (tm.tm_sec != last_sec) {
        last_sec = tm.tm_sec;
        lv_arc_set_value(ring, tm.tm_sec);

        /* blinking colon, once per second */
        if (tm.tm_sec & 1) {
            lv_obj_add_flag(colon_top, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colon_bot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(colon_top, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(colon_bot, LV_OBJ_FLAG_HIDDEN);
        }
    }

    static int last_h = -1;
    static int last_m = -1;
    static int last_s = -1;

    int hour_val = (tm.tm_hour % 12) * 50 + (tm.tm_min * 5) / 6;
    int min_val = tm.tm_min * 10 + tm.tm_sec / 6;
    int sec_val = tm.tm_sec * 10 + ms / 100;

    if (hour_val != last_h) {
        last_h = hour_val;
        lv_scale_set_line_needle_value(scale, hour_hand, 46, hour_val);
    }

    if (min_val != last_m) {
        last_m = min_val;
        lv_scale_set_line_needle_value(scale, min_hand, 70, min_val);
    }

    if (sec_val != last_s) {
        last_s = sec_val;
        lv_scale_set_line_needle_value(scale, sec_hand, 86, sec_val);
    }
}

static int active_page(void)
{
    lv_obj_t *active = lv_tileview_get_tile_active(tileview);

    for (int i = 0; i < NUM_PAGES; i++) {
        if (tiles[i] == active) {
            return i;
        }
    }

    return 0;
}

static void goto_page(int page)
{
    lv_tileview_set_tile(tileview, tiles[page], LV_ANIM_ON);
}

static void tile_changed_cb(lv_event_t *e)
{
    (void)e;

    int page = active_page();

    if (page == PAGE_ACTIVITY) {
        play_activity_anim();
    } else if (page == PAGE_APPS) {
        reset_focus();
    }
}

static void handle_press(bool is_long)
{
    if (open_app >= 0) {
        close_app_screen();
        return;
    }

    int page = active_page();

    if (page == PAGE_APPS) {
        if (is_long) {
            open_app_screen(menu_focus);
        } else if (menu_focus < MENU_COUNT - 1) {
            set_focus(menu_focus + 1);
        } else {
            goto_page(PAGE_DIGITAL);
        }

        return;
    }

    if (is_long) {
        goto_page(PAGE_APPS);
    } else {
        goto_page((page + 1) % NUM_PAGES);
    }
}

static void button_poll(lv_timer_t *t)
{
    static bool prev = true;
    static uint32_t t_down = 0;

    (void)t;

    bool level = gpio_get_level(PIN_SWITCH_BTN);

    if (!level && prev) {
        t_down = lv_tick_get();
    } else if (level && !prev) {
        handle_press(lv_tick_elaps(t_down) >= LONG_PRESS_MS);
    }

    prev = level;
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, COL_BG, 0);

    tileview = lv_tileview_create(scr);
    lv_obj_set_style_bg_color(tileview, COL_BG, 0);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    tiles[PAGE_DIGITAL] =
        lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);

    tiles[PAGE_ANALOG] =
        lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);

    tiles[PAGE_ACTIVITY] =
        lv_tileview_add_tile(tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);

    tiles[PAGE_APPS] =
        lv_tileview_add_tile(tileview, 3, 0, LV_DIR_LEFT);

    build_digital(tiles[PAGE_DIGITAL]);
    build_analog(tiles[PAGE_ANALOG]);
    build_activity(tiles[PAGE_ACTIVITY]);
    build_apps(tiles[PAGE_APPS]);
    build_app_overlay();

    lv_obj_add_event_cb(tileview, tile_changed_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_create(update_ui, 100, NULL);
    lv_timer_create(button_poll, 40, NULL);
}

void ui_init(void)
{
    gpio_config_t btn_cfg = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = 1ULL << PIN_SWITCH_BTN,
    };

    ESP_ERROR_CHECK(gpio_config(&btn_cfg));

    if (lvgl_port_lock(0)) {
        build_ui();
        lvgl_port_unlock();
    }
}