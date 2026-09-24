#include <stdbool.h>
#include <string.h>

#include "../include/menu/settings.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

#define SETTINGS_VISIBLE  5
#define SETTINGS_START_Y  46
#define SETTINGS_ITEM_H   32
#define SETTINGS_ROW_X    18
#define SETTINGS_ROW_W    204
#define SETTINGS_ROW_H    26

#define COLOR_BG          0x0000
#define COLOR_ROW         0x2104 // subtle dark grey, unselected row
#define COLOR_ROW_SEL     0x07FF // cyan, selected row
#define COLOR_TEXT        0xFFFF
#define COLOR_TEXT_SEL    0x0000
#define COLOR_TEXT_DIM    0x7BEF
#define COLOR_CARD        0x1082 // detail card background

static settings_item_t selected_setting = SETTINGS_DISPLAY;
static bool showing_detail = false;
static const char *detail_value = "";
static bool detail_is_toggle = false;
static bool detail_toggle_on = false;

static const char *settings_names[SETTINGS_COUNT] = {
    "Display",
    "Brightness",
    "Sound",
    "Vibration",
    "Bluetooth",
    "WiFi",
    "Battery",
    "About"
};

// a small accent colour per row, so each setting has a distinct identity
// at a glance rather than a uniform list, similar to smartwatch UIs
static const uint16_t settings_accent[SETTINGS_COUNT] = {
    0x07FF, // Display   - cyan
    0xFD20, // Brightness- amber
    0xF81F, // Sound     - magenta
    0x841F, // Vibration - violet
    0x3D7F, // Bluetooth - blue
    0x2FE6, // WiFi      - teal
    0x07E0, // Battery   - green
    0x8410, // About     - grey
};

static const char *settings_demo_values[SETTINGS_COUNT] = {
    "AUTO",
    "80%",
    "ON",
    "ON",
    "OFF",
    "CONNECTED",
    "87%",
    "FW V0.1"
};

// whether each setting is best shown as an ON/OFF toggle on its detail
// screen rather than plain text
static const bool settings_is_toggle[SETTINGS_COUNT] = {
    false, false, true, true, true, false, false, false
};

static const bool settings_toggle_state[SETTINGS_COUNT] = {
    false, false, true, true, false, false, false, false
};

static void draw_accent_dot(int cx, int cy, uint16_t color)
{
    circle dot = { .cen = { cx, cy }, .radius = 4, .color = color };
    draw_circle(&dot);
}

// small right-pointing chevron, drawn as two short diagonal strokes
static void draw_chevron(int x, int y, uint16_t color)
{
    draw_line(x, y - 5, x + 5, y, color);
    draw_line(x + 5, y, x, y + 5, color);
}

static void draw_toggle_switch(int x, int y, bool on, uint16_t on_color)
{
    int w = 44, h = 20;
    uint16_t track = on ? on_color : 0x4A69;

    draw_rounded_rect(x, y, w, h, h / 2, track);

    int knob_r = (h / 2) - 2;
    int knob_x = on ? (x + w - h / 2) : (x + h / 2);
    circle knob = { .cen = { knob_x, y + h / 2 }, .radius = knob_r, .color = 0xFFFF };
    draw_circle(&knob);
}

static void render_settings_list(void)
{
    fb_clear(COLOR_BG);

    draw_text_box(72, 10, 100, 20, "SETTINGS", COLOR_TEXT, 2);
    draw_line(30, 34, 210, 34, 0x2965);

    int start = (int)selected_setting - 2;

    if (start < 0)
        start = 0;

    if (start > SETTINGS_COUNT - SETTINGS_VISIBLE)
        start = SETTINGS_COUNT - SETTINGS_VISIBLE;

    for (int i = 0; i < SETTINGS_VISIBLE; i++) {

        int index = start + i;
        int y = SETTINGS_START_Y + i * SETTINGS_ITEM_H;
        bool selected = (index == selected_setting);

        draw_rounded_rect(
            SETTINGS_ROW_X, y,
            SETTINGS_ROW_W, SETTINGS_ROW_H,
            SETTINGS_ROW_H / 2,
            selected ? COLOR_ROW_SEL : COLOR_ROW
        );

        draw_accent_dot(SETTINGS_ROW_X + 16, y + SETTINGS_ROW_H / 2, settings_accent[index]);

        draw_text_box(
            SETTINGS_ROW_X + 30, y + (SETTINGS_ROW_H - 8) / 2,
            140, 16,
            settings_names[index],
            selected ? COLOR_TEXT_SEL : COLOR_TEXT,
            1
        );

        draw_chevron(
            SETTINGS_ROW_X + SETTINGS_ROW_W - 16,
            y + SETTINGS_ROW_H / 2,
            selected ? COLOR_TEXT_SEL : COLOR_TEXT_DIM
        );
    }

    if (selected_setting > 0) {
        draw_line(120, 36, 120, 40, COLOR_TEXT_DIM);
    }
    if (selected_setting < SETTINGS_COUNT - 1) {
        draw_line(120, 212, 120, 216, COLOR_TEXT_DIM);
    }
}

static void render_settings_detail(void)
{
    fb_clear(COLOR_BG);

    uint16_t accent = settings_accent[selected_setting];

    draw_accent_dot(120, 34, accent);
    draw_text_box(30, 48, 180, 22, settings_names[selected_setting], COLOR_TEXT, 2);
    draw_line(30, 76, 210, 76, 0x2965);

    draw_rounded_rect(35, 96, 170, 64, 14, COLOR_CARD);

    if (detail_is_toggle) {
        draw_text_box(50, 116, 100, 16, detail_toggle_on ? "ENABLED" : "DISABLED", COLOR_TEXT, 1);
        draw_toggle_switch(140, 128, detail_toggle_on, accent);
    } else {
        draw_text_box(50, 122, 140, 20, detail_value, accent, 1);
    }

    draw_text_box(60, 196, 130, 16, "PRESS TO BACK", COLOR_TEXT_DIM, 1);
}

void menu_settings_init(void)
{
    if (showing_detail) {
        render_settings_detail();
    } else {
        render_settings_list();
    }
}

void settings_navigate(void)
{
    if (showing_detail) {
        showing_detail = false;
        return;
    }

    selected_setting++;

    if (selected_setting >= SETTINGS_COUNT)
        selected_setting = SETTINGS_DISPLAY;
}

void settings_select(void)
{
    detail_value = settings_demo_values[selected_setting];
    detail_is_toggle = settings_is_toggle[selected_setting];
    detail_toggle_on = settings_toggle_state[selected_setting];
    showing_detail = true;
}