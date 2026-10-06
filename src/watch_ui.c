#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"

#include "menu.h"
#include "mock_data.h"
#include "specs.h"
#include "ui_common.h"
#include "watch_ui.h"

static const char *TAG = "watch_ui";

typedef enum {
    PAGE_DIGITAL,
    PAGE_ANALOG,
    PAGE_ACTIVITY,
    PAGE_APPS,
    PAGE_APP
} watch_page_t;

static const menu_app_t *apps[MENU_COUNT] = {
    &menu_fitness_app,
    &menu_phone_app,
    &menu_world_time_app,
    &menu_wallet_app,
    &menu_calendar_app,
    &menu_weather_app,
    &menu_workout_app,
    &menu_settings_app
};

#define ICON_BASE 54
#define ICON_FOCUS 64

/* ---- Analog face tuning ------------------------------------------------ */
#define GLOW_SIZE        120          /* glow is drawn at 120x120, shown at 2x */
#define HAND_CX          (LCD_H_RES / 2)
#define HAND_CY          (LCD_V_RES / 2)
#define HOUR_LEN         52
#define MIN_LEN          84
#define HOUR_W           7
#define MIN_W            6
#define COL_HAND_HOUR    lv_color_hex(0xBFF5D2)
#define COL_HAND_MIN     lv_color_hex(0xE9FFF1)
#define COL_LIME         lv_color_hex(0xD6F02A)
#define ANALOG_BATT_DEMO 80           /* demo battery percent */

/* ---- Digital face tuning ----------------------------------------------- */
/* lv_font_digits_92.c: digits-only Montserrat ExtraBold, 92 px (generated). */
LV_FONT_DECLARE(lv_font_digits_92);
#define DIGI_GAP         32           /* +/- y offset of hour / minute rows */
#define DIGI_BATT_DEMO   30           /* demo battery percent */
#define COL_DIGI_HOUR    lv_color_hex(0xF4F6F5)
#define COL_DIGI_MIN     lv_color_hex(0x35E8A5)
#define COL_DIGI_TRACK   lv_color_hex(0x0E1A16)

static watch_page_t current_page = PAGE_DIGITAL;
static int selected_app = 0;

static lv_obj_t *screen;
static lv_obj_t *digital_page;
static lv_obj_t *analog_page;
static lv_obj_t *activity_page;
static lv_obj_t *apps_page;
static lv_obj_t *app_page;

static lv_obj_t *digital_ring;
static lv_obj_t *digital_hour;
static lv_obj_t *digital_min;
static lv_obj_t *digital_day;
static lv_obj_t *digital_date;

static lv_obj_t *analog_glow;
static lv_obj_t *analog_lbl_a;     /* "SAT 10:09:" */
static lv_obj_t *analog_lbl_sec;   /* "24" (highlighted) */
static lv_obj_t *analog_lbl_b;     /* " AUG 16" */
static lv_obj_t *hour_hand;
static lv_obj_t *min_hand;

static lv_point_precise_t hour_pts[2];
static lv_point_precise_t min_pts[2];

LV_DRAW_BUF_DEFINE_STATIC(glow_draw_buf, GLOW_SIZE, GLOW_SIZE, LV_COLOR_FORMAT_RGB565);

static lv_obj_t *activity_arcs[3];

static lv_obj_t *launcher_icons[MENU_COUNT];
static lv_obj_t *app_title;

static lv_timer_t *ui_timer;
static lv_timer_t *button_timer;

static bool button_down;
static uint32_t button_start;
static bool long_press_sent;

static void set_page_hidden(lv_obj_t *obj, bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t *make_hand(lv_obj_t *parent, lv_point_precise_t *pts,
                           int width, lv_color_t color)
{
    lv_obj_t *hand = lv_line_create(parent);
    lv_obj_remove_style_all(hand);
    lv_obj_set_size(hand, LCD_H_RES, LCD_V_RES);
    lv_obj_set_pos(hand, 0, 0);
    lv_line_set_points_mutable(hand, pts, 2);
    lv_obj_set_style_line_width(hand, width, 0);
    lv_obj_set_style_line_color(hand, color, 0);
    lv_obj_set_style_line_rounded(hand, true, 0);
    return hand;
}

static void digital_build(void)
{
    digital_page = lv_obj_create(screen);
    lv_obj_remove_style_all(digital_page);
    lv_obj_set_size(digital_page, LCD_H_RES, LCD_V_RES);
    lv_obj_set_style_bg_color(digital_page, COL_BG, 0);
    lv_obj_set_style_bg_opa(digital_page, LV_OPA_COVER, 0);

    lv_obj_set_style_bg_color(digital_page, lv_color_black(), 0);

    /* Thin seconds ring around the edge */
    digital_ring = ui_ring(
        digital_page,
        232,
        4,
        COL_DIGI_MIN,
        COL_DIGI_TRACK,
        59
    );

    /* Huge stacked digits: hour (white) over minutes (mint) */
    digital_hour = lv_label_create(digital_page);
    lv_label_set_text(digital_hour, "--");
    lv_obj_set_style_text_font(digital_hour, &lv_font_digits_92, 0);
    lv_obj_set_style_text_color(digital_hour, COL_DIGI_HOUR, 0);
    lv_obj_align(digital_hour, LV_ALIGN_CENTER, 0, -DIGI_GAP);

    digital_min = lv_label_create(digital_page);
    lv_label_set_text(digital_min, "--");
    lv_obj_set_style_text_font(digital_min, &lv_font_digits_92, 0);
    lv_obj_set_style_text_color(digital_min, COL_DIGI_MIN, 0);
    lv_obj_align(digital_min, LV_ALIGN_CENTER, 0, DIGI_GAP);

    /* Weekday, rotated, on the left edge between the two rows */
    digital_day = lv_label_create(digital_page);
    lv_label_set_text(digital_day, "");
    lv_obj_set_style_text_font(digital_day, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(digital_day, COL_DIGI_MIN, 0);
    lv_obj_align(digital_day, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_update_layout(digital_day);
    lv_obj_set_style_transform_pivot_x(digital_day,
        lv_obj_get_width(digital_day) / 2, 0);
    lv_obj_set_style_transform_pivot_y(digital_day,
        lv_obj_get_height(digital_day) / 2, 0);
    lv_obj_set_style_transform_rotation(digital_day, -900, 0);

    /* Bottom row: "AUG.16" + battery */
    lv_obj_t *row = lv_obj_create(digital_page);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 92);

    digital_date = lv_label_create(row);
    lv_label_set_text(digital_date, "");
    lv_obj_set_style_text_font(digital_date, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(digital_date, lv_color_white(), 0);

    lv_obj_t *batt = lv_label_create(row);
    lv_label_set_text_fmt(batt, LV_SYMBOL_BATTERY_1 " %d%%", DIGI_BATT_DEMO);
    lv_obj_set_style_text_font(batt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(batt, lv_color_hex(0x9AA3A0), 0);

    ui_make_inert(digital_page);
}

static void digital_clock_update(const struct tm *tm, bool synced)
{
    static int last_hour = -2;
    static int last_min = -2;
    static int last_day = -2;
    char buf[16];

    int hour = synced ? tm->tm_hour : -1;
    int min = synced ? tm->tm_min : -1;
    int day = synced ? tm->tm_yday : -1;

    if (hour != last_hour) {
        last_hour = hour;
        if (synced) {
            snprintf(buf, sizeof(buf), "%02d", hour);
        } else {
            snprintf(buf, sizeof(buf), "--");
        }
        lv_label_set_text(digital_hour, buf);
    }

    if (min != last_min) {
        last_min = min;
        if (synced) {
            snprintf(buf, sizeof(buf), "%02d", min);
        } else {
            snprintf(buf, sizeof(buf), "--");
        }
        lv_label_set_text(digital_min, buf);
    }

    if (day != last_day) {
        last_day = day;

        if (synced) {
            char d[8];
            char dt[16];

            strftime(d, sizeof(d), "%a", tm);
            strftime(dt, sizeof(dt), "%b.%d", tm);

            for (char *p = dt; *p; p++) *p = (char)toupper((unsigned char)*p);

            lv_label_set_text(digital_day, d);
            lv_label_set_text(digital_date, dt);
        } else {
            lv_label_set_text(digital_day, "");
            lv_label_set_text(digital_date, "SYNCING");
        }

        /* label width changed: keep the rotation pivot on its centre */
        lv_obj_update_layout(digital_day);
        lv_obj_set_style_transform_pivot_x(digital_day,
            lv_obj_get_width(digital_day) / 2, 0);
        lv_obj_set_style_transform_pivot_y(digital_day,
            lv_obj_get_height(digital_day) / 2, 0);
    }

    lv_arc_set_value(digital_ring, synced ? tm->tm_sec : 0);
}

/* ======================= Analog face ==================================== */

static float glow_falloff(float d, float r)
{
    float t = 1.0f - d / r;

    if (t <= 0.0f) {
        return 0.0f;
    }

    return t * t * (3.0f - 2.0f * t);   /* smoothstep */
}

/* Paints the soft green (top) + yellow (bottom) blob once, at build time. */
static void glow_render(lv_obj_t *canvas)
{
    const float green_r = 60.0f, green_g = 175.0f, green_b = 120.0f;
    const float yel_r = 235.0f, yel_g = 245.0f, yel_b = 25.0f;

    for (int y = 0; y < GLOW_SIZE; y++) {
        float ny = ((y + 0.5f) / GLOW_SIZE) * 2.0f - 1.0f;

        for (int x = 0; x < GLOW_SIZE; x++) {
            float nx = ((x + 0.5f) / GLOW_SIZE) * 2.0f - 1.0f;

            float dg = sqrtf(nx * nx + (ny + 0.15f) * (ny + 0.15f));
            float dy = sqrtf(nx * nx + (ny - 0.30f) * (ny - 0.30f));

            float ig = glow_falloff(dg, 0.62f);
            float iy = glow_falloff(dy, 0.42f);

            float r = green_r * ig;
            float g = green_g * ig;
            float b = green_b * ig;

            r += (yel_r - r) * iy;
            g += (yel_g - g) * iy;
            b += (yel_b - b) * iy;

            lv_canvas_set_px(
                canvas, x, y,
                lv_color_make((uint8_t)r, (uint8_t)g, (uint8_t)b),
                LV_OPA_COVER
            );
        }
    }
}

static void analog_hands_set(int hour, int minute)
{
    int hour_deg = (hour % 12) * 30 + minute / 2;
    int min_deg = minute * 6;

    hour_pts[0].x = HAND_CX;
    hour_pts[0].y = HAND_CY;
    hour_pts[1].x = HAND_CX + (HOUR_LEN * (int)lv_trigo_sin(hour_deg)) / 32767;
    hour_pts[1].y = HAND_CY - (HOUR_LEN * (int)lv_trigo_cos(hour_deg)) / 32767;

    min_pts[0].x = HAND_CX;
    min_pts[0].y = HAND_CY;
    min_pts[1].x = HAND_CX + (MIN_LEN * (int)lv_trigo_sin(min_deg)) / 32767;
    min_pts[1].y = HAND_CY - (MIN_LEN * (int)lv_trigo_cos(min_deg)) / 32767;

    lv_obj_invalidate(hour_hand);
    lv_obj_invalidate(min_hand);
}

static void analog_side_arc(lv_obj_t *parent, int start, int end,
                            int value, bool reverse)
{
    lv_obj_t *arc = lv_arc_create(parent);

    lv_obj_set_size(arc, 226, 226);
    lv_obj_center(arc);
    lv_arc_set_bg_angles(arc, start, end);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, value);

    if (reverse) {
        lv_arc_set_mode(arc, LV_ARC_MODE_REVERSE);
    }

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x2A2D31), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0xDCE6E0), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
}

static lv_obj_t *analog_side_text(lv_obj_t *parent, const char *text,
                                  lv_align_t align, int x, int y,
                                  int rotation)
{
    lv_obj_t *lbl = lv_label_create(parent);

    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xDCE6E0), 0);
    lv_obj_align(lbl, align, x, y);

    lv_obj_update_layout(lbl);
    lv_obj_set_style_transform_pivot_x(lbl, lv_obj_get_width(lbl) / 2, 0);
    lv_obj_set_style_transform_pivot_y(lbl, lv_obj_get_height(lbl) / 2, 0);
    lv_obj_set_style_transform_rotation(lbl, rotation, 0);

    return lbl;
}

static lv_obj_t *analog_date_part(lv_obj_t *row, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(row);

    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, color, 0);

    return lbl;
}

static void analog_build(void)
{
    char buf[16];

    analog_page = lv_obj_create(screen);
    lv_obj_remove_style_all(analog_page);
    lv_obj_set_size(analog_page, LCD_H_RES, LCD_V_RES);
    lv_obj_set_style_bg_color(analog_page, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(analog_page, LV_OPA_COVER, 0);

    /* 1) Soft glow: small canvas, scaled 2x so the blur stays smooth. */
    LV_DRAW_BUF_INIT_STATIC(glow_draw_buf);

    analog_glow = lv_canvas_create(analog_page);
    lv_canvas_set_draw_buf(analog_glow, &glow_draw_buf);
    glow_render(analog_glow);
    lv_obj_center(analog_glow);
    lv_image_set_pivot(analog_glow, GLOW_SIZE / 2, GLOW_SIZE / 2);
    lv_image_set_scale(analog_glow, 512);

    /* 2) Side indicators: battery (left), heart rate (right). */
    analog_side_arc(analog_page, 150, 210, ANALOG_BATT_DEMO, false);

    int bpm_pct = MOCK_BPM * 100 / 200;
    if (bpm_pct > 100) bpm_pct = 100;
    if (bpm_pct < 0)   bpm_pct = 0;
    analog_side_arc(analog_page, 330, 30, bpm_pct, true);

    snprintf(buf, sizeof(buf), "%%%d", ANALOG_BATT_DEMO);
    analog_side_text(analog_page, buf, LV_ALIGN_LEFT_MID, 18, 22, -900);

    snprintf(buf, sizeof(buf), "%d", MOCK_BPM);
    analog_side_text(analog_page, buf, LV_ALIGN_RIGHT_MID, -18, 22, 900);

    lv_obj_t *batt = lv_label_create(analog_page);
    lv_label_set_text(batt, LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_font(batt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(batt, lv_color_hex(0xDCE6E0), 0);
    lv_obj_align(batt, LV_ALIGN_CENTER, -82, -34);

    lv_obj_t *drop = lv_obj_create(analog_page);
    lv_obj_remove_style_all(drop);
    lv_obj_set_size(drop, 7, 7);
    lv_obj_set_style_radius(drop, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(drop, lv_color_hex(0xDCE6E0), 0);
    lv_obj_set_style_bg_opa(drop, LV_OPA_COVER, 0);
    lv_obj_align(drop, LV_ALIGN_CENTER, 82, -34);

    /* 3) Bottom date line: "SAT 10:09:" + highlighted seconds + " AUG 16" */
    lv_obj_t *row = lv_obj_create(analog_page);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 0, 0);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 84);

    analog_lbl_a = analog_date_part(row, lv_color_white());
    analog_lbl_sec = analog_date_part(row, COL_LIME);
    analog_lbl_b = analog_date_part(row, lv_color_white());

    /* 4) Hands (two, no second hand - seconds are shown as digits). */
    hour_hand = make_hand(analog_page, hour_pts, HOUR_W, COL_HAND_HOUR);
    min_hand = make_hand(analog_page, min_pts, MIN_W, COL_HAND_MIN);
    analog_hands_set(10, 10);

    /* 5) Pivot cap. */
    lv_obj_t *cap = lv_obj_create(analog_page);
    lv_obj_remove_style_all(cap);
    lv_obj_set_size(cap, 13, 13);
    lv_obj_set_style_radius(cap, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cap, COL_HAND_MIN, 0);
    lv_obj_set_style_bg_opa(cap, LV_OPA_COVER, 0);
    lv_obj_center(cap);

    lv_obj_t *cap_in = lv_obj_create(cap);
    lv_obj_remove_style_all(cap_in);
    lv_obj_set_size(cap_in, 5, 5);
    lv_obj_set_style_radius(cap_in, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cap_in, lv_color_hex(0x1C3A2C), 0);
    lv_obj_set_style_bg_opa(cap_in, LV_OPA_COVER, 0);
    lv_obj_center(cap_in);

    ui_make_inert(analog_page);
}

static void analog_clock_update(const struct tm *tm, bool synced)
{
    static int last_sec = -1;
    static int last_min = -1;
    static bool last_synced = true;

    if (!synced) {
        if (last_synced) {
            last_synced = false;
            last_sec = -1;
            last_min = -1;
            analog_hands_set(10, 10);
            lv_label_set_text(analog_lbl_a, "SYNCING");
            lv_label_set_text(analog_lbl_sec, "");
            lv_label_set_text(analog_lbl_b, "");
        }
        return;
    }

    last_synced = true;

    if (tm->tm_min != last_min) {
        last_min = tm->tm_min;
        analog_hands_set(tm->tm_hour, tm->tm_min);
    }

    if (tm->tm_sec != last_sec) {
        char a[24];
        char s[8];
        char b[24];

        last_sec = tm->tm_sec;

        strftime(a, sizeof(a), "%a %H:%M:", tm);
        strftime(s, sizeof(s), "%S", tm);
        strftime(b, sizeof(b), " %b %d", tm);

        for (char *p = a; *p; p++) *p = (char)toupper((unsigned char)*p);
        for (char *p = b; *p; p++) *p = (char)toupper((unsigned char)*p);

        lv_label_set_text(analog_lbl_a, a);
        lv_label_set_text(analog_lbl_sec, s);
        lv_label_set_text(analog_lbl_b, b);
    }
}

/* ======================= Activity page ================================== */

static void activity_build(void)
{
    activity_page = lv_obj_create(screen);
    lv_obj_remove_style_all(activity_page);
    lv_obj_set_size(activity_page, LCD_H_RES, LCD_V_RES);
    lv_obj_set_style_bg_color(activity_page, COL_BG, 0);
    lv_obj_set_style_bg_opa(activity_page, LV_OPA_COVER, 0);

    activity_arcs[0] = ui_ring(
        activity_page,
        226,
        18,
        lv_color_hex(0xFF2D55),
        lv_color_hex(0x33101A),
        100
    );

    activity_arcs[1] = ui_ring(
        activity_page,
        182,
        18,
        lv_color_hex(0x92E82A),
        lv_color_hex(0x1D2E0D),
        100
    );

    activity_arcs[2] = ui_ring(
        activity_page,
        138,
        18,
        lv_color_hex(0x00C7FF),
        lv_color_hex(0x00252E),
        100
    );

    ui_label(
        activity_page,
        &lv_font_montserrat_28,
        COL_TEXT,
        "412",
        LV_ALIGN_CENTER,
        0,
        -8
    );

    ui_label(
        activity_page,
        &lv_font_montserrat_14,
        COL_MUTED,
        "kcal",
        LV_ALIGN_CENTER,
        0,
        18
    );

    ui_make_inert(activity_page);
}

static void activity_anim_cb(void *obj, int32_t value)
{
    lv_arc_set_value(obj, value);
}

static void activity_show(void)
{
    static const int targets[3] = { 78, 54, 91 };

    for (int i = 0; i < 3; i++) {
        lv_anim_t anim;

        lv_anim_init(&anim);
        lv_anim_set_var(&anim, activity_arcs[i]);
        lv_anim_set_exec_cb(&anim, activity_anim_cb);
        lv_anim_set_values(&anim, 0, targets[i]);
        lv_anim_set_duration(&anim, 900);
        lv_anim_set_delay(&anim, i * 140);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_start(&anim);
    }
}

/* ======================= Launcher ======================================= */

static void launcher_position(int index)
{
    static const int x[MENU_COUNT] = {
        -62, 0, 62,
        -31, 31,
        -62, 0, 62
    };

    static const int y[MENU_COUNT] = {
        -54, -54, -54,
        0, 0,
        54, 54, 54
    };

    lv_obj_align(
        launcher_icons[index],
        LV_ALIGN_CENTER,
        x[index],
        y[index]
    );
}

static void launcher_icon_build(int index)
{
    lv_obj_t *icon = lv_obj_create(apps_page);

    lv_obj_set_size(
        icon,
        ICON_BASE,
        ICON_BASE
    );

    lv_obj_set_style_radius(
        icon,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        icon,
        lv_color_hex(apps[index]->color),
        0
    );

    lv_obj_set_style_bg_opa(
        icon,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        icon,
        0,
        0
    );

    lv_obj_set_style_pad_all(
        icon,
        0,
        0
    );

    ui_make_inert(icon);

    apps[index]->build_icon(icon);

    launcher_icons[index] = icon;
    launcher_position(index);
}

static void launcher_build(void)
{
    apps_page = lv_obj_create(screen);
    lv_obj_remove_style_all(apps_page);
    lv_obj_set_size(apps_page, LCD_H_RES, LCD_V_RES);
    lv_obj_set_style_bg_color(apps_page, COL_BG, 0);
    lv_obj_set_style_bg_opa(apps_page, LV_OPA_COVER, 0);

    for (int i = 0; i < MENU_COUNT; i++) {
        launcher_icon_build(i);
    }

    ui_make_inert(apps_page);
}

static void launcher_set_focus(int index)
{
    for (int i = 0; i < MENU_COUNT; i++) {
        lv_obj_set_style_border_width(
            launcher_icons[i],
            0,
            0
        );

        lv_obj_set_size(
            launcher_icons[i],
            ICON_BASE,
            ICON_BASE
        );
    }

    selected_app = index;

    lv_obj_set_style_border_width(
        launcher_icons[selected_app],
        3,
        0
    );

    lv_obj_set_style_border_color(
        launcher_icons[selected_app],
        COL_TEXT,
        0
    );

    lv_obj_set_size(
        launcher_icons[selected_app],
        ICON_FOCUS,
        ICON_FOCUS
    );

    lv_obj_move_foreground(
        launcher_icons[selected_app]
    );
}

static void launcher_reset(void)
{
    launcher_set_focus(0);
}

/* ======================= App pages ====================================== */

static void app_open(void)
{
    if (selected_app < 0 || selected_app >= MENU_COUNT) {
        return;
    }

    if (apps[selected_app] == NULL ||
        apps[selected_app]->open == NULL) {
        return;
    }

    app_page = lv_obj_create(screen);
    lv_obj_remove_style_all(app_page);

    lv_obj_set_size(
        app_page,
        LCD_H_RES,
        LCD_V_RES
    );

    lv_obj_set_style_bg_color(
        app_page,
        COL_BG,
        0
    );

    lv_obj_set_style_bg_opa(
        app_page,
        LV_OPA_COVER,
        0
    );

    if (!apps[selected_app]->fullscreen) {
        app_title = ui_label(
            app_page,
            &lv_font_montserrat_20,
            COL_TEXT,
            apps[selected_app]->title,
            LV_ALIGN_TOP_MID,
            0,
            15
        );
    }

    apps[selected_app]->open(app_page);

    current_page = PAGE_APP;

    set_page_hidden(digital_page, true);
    set_page_hidden(analog_page, true);
    set_page_hidden(activity_page, true);
    set_page_hidden(apps_page, true);
    set_page_hidden(app_page, false);
}

static void app_close(void)
{
    if (selected_app >= 0 &&
        selected_app < MENU_COUNT &&
        apps[selected_app] != NULL &&
        apps[selected_app]->close != NULL) {
        apps[selected_app]->close();
    }

    if (app_page != NULL) {
        lv_obj_delete(app_page);
        app_page = NULL;
    }

    app_title = NULL;
}

static void show_page(watch_page_t page)
{
    if (page == PAGE_APP) {
        app_open();
        return;
    }

    if (app_page != NULL) {
        app_close();
    }

    current_page = page;

    set_page_hidden(digital_page, true);
    set_page_hidden(analog_page, true);
    set_page_hidden(activity_page, true);
    set_page_hidden(apps_page, true);

    if (page == PAGE_DIGITAL) {
        set_page_hidden(digital_page, false);
    } else if (page == PAGE_ANALOG) {
        set_page_hidden(analog_page, false);
    } else if (page == PAGE_ACTIVITY) {
        set_page_hidden(activity_page, false);
        activity_show();
    } else if (page == PAGE_APPS) {
        set_page_hidden(apps_page, false);
        launcher_reset();
    }
}

static void next_page(void)
{
    switch (current_page) {
    case PAGE_DIGITAL:
        show_page(PAGE_ANALOG);
        break;

    case PAGE_ANALOG:
        show_page(PAGE_ACTIVITY);
        break;

    case PAGE_ACTIVITY:
        show_page(PAGE_APPS);
        break;

    case PAGE_APPS:
        if (selected_app < MENU_COUNT - 1) {
            launcher_set_focus(selected_app + 1);
        } else {
            show_page(PAGE_DIGITAL);
        }
        break;

    case PAGE_APP:
        app_close();
        show_page(PAGE_APPS);
        break;
    }
}

static void button_short_press(void)
{
    if (current_page == PAGE_APP) {
        app_close();
        show_page(PAGE_APPS);
        return;
    }

    if (current_page == PAGE_APPS) {
        next_page();
        return;
    }

    next_page();
}

static void button_long_press(void)
{
    if (current_page == PAGE_APP) {
        app_close();
        show_page(PAGE_APPS);
        return;
    }

    if (current_page == PAGE_APPS) {
        app_open();
        return;
    }

    show_page(PAGE_APPS);
}

static void button_update(lv_timer_t *timer)
{
    (void)timer;

    bool pressed = gpio_get_level(PIN_SWITCH_BTN) == 0;

    if (pressed && !button_down) {
        button_down = true;
        button_start = lv_tick_get();
        long_press_sent = false;
    }

    if (pressed && button_down && !long_press_sent) {
        uint32_t elapsed = lv_tick_elaps(button_start);

        if (elapsed >= LONG_PRESS_MS) {
            long_press_sent = true;
        }
    }

    if (!pressed && button_down) {
        button_down = false;

        uint32_t elapsed = lv_tick_elaps(button_start);

        if (elapsed >= LONG_PRESS_MS) {
            button_long_press();
        } else {
            button_short_press();
        }
    }
}

static void clock_update(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;
    struct tm tm;

    localtime_r(&now, &tm);

    bool synced = tm.tm_year >= (2024 - 1900);

    analog_clock_update(&tm, synced);
    digital_clock_update(&tm, synced);
}

static void minute_update(void)
{
    if (!ui_time_synced()) {
        return;
    }

    time_t now = time(NULL);
    struct tm tm;

    localtime_r(&now, &tm);

    for (int i = 0; i < MENU_COUNT; i++) {
        if (apps[i] != NULL &&
            apps[i]->on_minute != NULL) {
            apps[i]->on_minute(&tm, true);
        }
    }
}

static void ui_update(lv_timer_t *timer)
{
    (void)timer;

    static int last_minute = -1;

    clock_update();

    if (current_page == PAGE_APP &&
        selected_app >= 0 &&
        selected_app < MENU_COUNT &&
        apps[selected_app] != NULL &&
        apps[selected_app]->refresh != NULL) {
        apps[selected_app]->refresh();
    }

    if (ui_time_synced()) {
        time_t now = time(NULL);
        struct tm tm;

        localtime_r(&now, &tm);

        if (tm.tm_min != last_minute) {
            last_minute = tm.tm_min;
            minute_update();
        }
    }
}

void watch_ui_init(void)
{
    gpio_config_t button_cfg = {
        .pin_bit_mask = 1ULL << PIN_SWITCH_BTN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&button_cfg));

    screen = lv_scr_act();

    lv_obj_set_style_bg_color(
        screen,
        COL_BG,
        0
    );

    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        0
    );

    digital_build();
    analog_build();
    activity_build();
    launcher_build();

    show_page(PAGE_DIGITAL);

    ui_timer = lv_timer_create(
        ui_update,
        100,
        NULL
    );

    button_timer = lv_timer_create(
        button_update,
        40,
        NULL
    );

    minute_update();

    ESP_LOGI(TAG, "UI initialized");
}