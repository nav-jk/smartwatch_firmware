#include <stdio.h>
#include <time.h>

#include "menu.h"
#include "ui_common.h"

#define CAL_RED      0xFF3B30
#define CAL_TXT      0xFFFFFF
#define CAL_DIM      0x8E8E93
#define CAL_WEEKEND  0xFF6B60
#define CAL_BG       0x000000

/* Size / position tuning */
#define GRID_W_PCT   68   /* grid width as % of page width */
#define TOP_PAD_PCT  17   /* push everything down: raise this to move lower */
#define BOT_PAD_PCT  4    /* keep-clear space at the bottom */
#define HDR_H        16
#define DOW_H        12
#define GAP          3

#define FONT_HDR     (&lv_font_montserrat_14)
#define FONT_DOW     (&lv_font_montserrat_14)
#define FONT_DAY     (&lv_font_montserrat_14)

static const char *dow_names[7] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};
static const char *dow_initial[7] = { "S", "M", "T", "W", "T", "F", "S" };
static const char *month_names[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

/* Launcher icon labels live as long as the launcher */
static lv_obj_t *icon_dow;
static lv_obj_t *icon_day;

/* Page widgets, valid only while the page is open */
static lv_obj_t *body;
static lv_obj_t *hdr_month;
static lv_obj_t *hdr_year;
static lv_obj_t *cell[42];
static lv_obj_t *cell_lbl[42];

static void calendar_icon(lv_obj_t *icon)
{
    icon_dow = ui_label(icon, &lv_font_montserrat_14, lv_color_hex(CAL_RED),
                        "---", LV_ALIGN_CENTER, 0, -11);
    icon_day = ui_label(icon, &lv_font_montserrat_20, COL_DARK,
                        "--", LV_ALIGN_CENTER, 0, 8);
}

static void calendar_on_minute(const struct tm *tm, bool synced)
{
    if (synced) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", tm->tm_mday);
        ui_set_text(icon_dow, dow_names[tm->tm_wday % 7]);
        ui_set_text(icon_day, buf);
    } else {
        ui_set_text(icon_dow, "---");
        ui_set_text(icon_day, "--");
    }
}

static int days_in_month(int year, int month /* 0-11 */)
{
    static const int d[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 1) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return leap ? 29 : 28;
    }
    return d[month];
}

static void calendar_refresh(void)
{
    if (!body) return;

    if (!ui_time_synced()) {
        ui_set_text(hdr_month, "Syncing...");
        ui_set_text(hdr_year, "");
        for (int i = 0; i < 42; i++) {
            lv_obj_set_style_bg_opa(cell[i], LV_OPA_TRANSP, 0);
            ui_set_text(cell_lbl[i], "");
        }
        return;
    }

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    char buf[16];
    ui_set_text(hdr_month, month_names[tm.tm_mon % 12]);
    snprintf(buf, sizeof(buf), "%d", tm.tm_year + 1900);
    ui_set_text(hdr_year, buf);

    int first = (((tm.tm_wday - (tm.tm_mday - 1)) % 7) + 7) % 7;
    int dim = days_in_month(tm.tm_year + 1900, tm.tm_mon);

    for (int i = 0; i < 42; i++) {
        int day = i - first + 1;
        bool valid = (day >= 1 && day <= dim);
        bool today = valid && day == tm.tm_mday;
        bool weekend = (i % 7) == 0 || (i % 7) == 6;

        if (valid) {
            snprintf(buf, sizeof(buf), "%d", day);
            ui_set_text(cell_lbl[i], buf);
        } else {
            ui_set_text(cell_lbl[i], "");
        }

        lv_obj_set_style_bg_opa(cell[i], today ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(cell_lbl[i],
            lv_color_hex(today ? CAL_TXT : (weekend ? CAL_WEEKEND : CAL_TXT)), 0);
    }
}

static void calendar_open(lv_obj_t *page)
{
    body = page;

    lv_obj_set_style_bg_color(page, lv_color_hex(CAL_BG), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_update_layout(page);
    int W = lv_obj_get_width(page);
    int H = lv_obj_get_height(page);

    int y = H * TOP_PAD_PCT / 100;
    int bottom = H * BOT_PAD_PCT / 100;

    /* Cell size: limited by width AND by the height left below the header */
    int avail = H - y - bottom - HDR_H - DOW_H - GAP;
    int cs_w  = (W * GRID_W_PCT / 100) / 7;
    int cs_h  = avail / 6;
    int cs    = cs_w < cs_h ? cs_w : cs_h;
    if (cs < 12) cs = 12;

    int gw = cs * 7;
    int ox = (W - gw) / 2;

    /* Header */
    hdr_month = ui_label(page, FONT_HDR, lv_color_hex(CAL_TXT),
                         "", LV_ALIGN_TOP_LEFT, ox + 2, y);
    hdr_year  = ui_label(page, FONT_DOW, lv_color_hex(CAL_RED),
                         "", LV_ALIGN_TOP_RIGHT, -(ox + 2), y + 2);
    y += HDR_H;

    /* Weekday initials */
    for (int c = 0; c < 7; c++) {
        lv_obj_t *l = ui_label(page, FONT_DOW, lv_color_hex(CAL_DIM),
                               dow_initial[c], LV_ALIGN_TOP_LEFT, 0, y);
        lv_obj_update_layout(l);
        lv_obj_set_x(l, ox + c * cs + (cs - lv_obj_get_width(l)) / 2);
    }
    y += DOW_H;

    /* Day grid */
    int pad = 1;
    for (int i = 0; i < 42; i++) {
        lv_obj_t *c = lv_obj_create(page);
        lv_obj_remove_style_all(c);
        lv_obj_set_size(c, cs - pad * 2, cs - pad * 2);
        lv_obj_set_pos(c, ox + (i % 7) * cs + pad, y + (i / 7) * cs + pad);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(c, lv_color_hex(CAL_RED), 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
        cell[i] = c;
        cell_lbl[i] = ui_label(c, FONT_DAY, lv_color_hex(CAL_TXT), "",
                               LV_ALIGN_CENTER, 0, 0);
    }

    calendar_refresh();
}

static void calendar_close(void)
{
    body = NULL;
    hdr_month = hdr_year = NULL;
    for (int i = 0; i < 42; i++) cell[i] = cell_lbl[i] = NULL;
}

const menu_app_t menu_calendar_app = {
    .title = "Calendar",
    .color = 0xF2F2F7,
    .build_icon = calendar_icon,
    .open = calendar_open,
    .refresh = calendar_refresh,
    .close = calendar_close,
    .on_minute = calendar_on_minute,
};