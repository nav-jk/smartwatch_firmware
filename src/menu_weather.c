#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "cJSON.h"

#include "menu.h"
#include "ui_common.h"

#define WEATHER_LAT "11.321973"
#define WEATHER_LON "75.935386"

#define WEATHER_URL \
    "https://api.open-meteo.com/v1/forecast?" \
    "latitude=" WEATHER_LAT \
    "&longitude=" WEATHER_LON \
    "&current=temperature_2m,weather_code" \
    "&daily=weather_code" \
    "&forecast_days=4" \
    "&temperature_unit=celsius" \
    "&timezone=Asia%2FKolkata"

#define WEATHER_UPDATE_MS (10 * 60 * 1000)
#define WEATHER_CITY      "NIT Calicut"

/* Colors (WX_ prefix so they never clash with ui_common.h) */
#define WX_TXT   0xFFFFFF
#define WX_DIM   0xB4BCC8

/* Layout (percent of page size) */
#define TEMP_X_PCT    22
#define TEMP_Y_PCT    13
#define ICON_X_PCT    58
#define ICON_Y_PCT    16
#define ICON_SIZE     46
#define CITY_Y_PCT    38
#define COND_Y_PCT    48
#define DATE_Y_PCT    55
#define CARDS_Y_PCT   67
#define CARD_H        48
#define CARD_GAP      4
#define EDGE_MARGIN   6
#define FC_ICON       22

/* Big temperature font: uses Montserrat 48 if enabled, else 28 */
#if defined(LV_FONT_MONTSERRAT_48) && LV_FONT_MONTSERRAT_48
#define FONT_TEMP     (&lv_font_montserrat_48)
#define RING_SIZE     11
#define RING_TOP      7
#else
#define FONT_TEMP     (&lv_font_montserrat_28)
#define RING_SIZE     9
#define RING_TOP      4
#endif

static const char *TAG = "weather";

typedef enum { WX_LOADING, WX_OK, WX_ERROR } wx_state_t;

typedef struct {
    wx_state_t state;
    float temp;
    int code;
    int fc_code[3];     /* next three days */
} wx_data_t;

/* Kept across page open/close so reopening shows data immediately */
static wx_data_t wx = { .state = WX_LOADING, .code = -1,
                        .fc_code = { -1, -1, -1 } };

static lv_obj_t *body;          /* page root, NULL when closed */
static lv_obj_t *temp_lbl, *cond_lbl, *date_lbl;
static lv_obj_t *big_icon;
static lv_obj_t *fc_icon[3], *fc_day[3];

static char response_buffer[2048];
static int response_length;

static TaskHandle_t weather_task_handle;

static const char *dow_short[7] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static const char *weather_description(int code)
{
    switch (code) {
    case 0:  return "Clear sky";
    case 1:  return "Mainly clear";
    case 2:  return "Partly cloudy";
    case 3:  return "Overcast";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 56:
    case 57: return "Freezing drizzle";
    case 61:
    case 63:
    case 65: return "Rain";
    case 66:
    case 67: return "Freezing rain";
    case 71:
    case 73:
    case 75: return "Snow";
    case 77: return "Snow grains";
    case 80:
    case 81:
    case 82: return "Rain showers";
    case 85:
    case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Thunder + hail";
    default: return "Unknown";
    }
}

/* Top color of the background gradient (fades to black at the bottom) */
static uint32_t weather_bg(int code)
{
    if (code < 0)   return 0x1C2A44;
    if (code <= 1)  return 0x1F5FB8;   /* clear */
    if (code <= 3)  return 0x3B5272;   /* cloudy */
    if (code <= 48) return 0x4A4F57;   /* fog */
    if (code <= 67) return 0x1B3A6B;   /* rain */
    if (code <= 77) return 0x4F6D8C;   /* snow */
    if (code <= 86) return 0x1B3A6B;   /* showers / snow showers */
    return 0x3E2C66;                   /* storm */
}

/* ---------- icon drawing (all LVGL shapes, no image assets) ---------- */

typedef enum {
    K_CLOUD, K_SUN, K_PARTLY, K_FOG, K_RAIN, K_SNOW, K_STORM
} wx_kind_t;

static wx_kind_t weather_kind(int code)
{
    if (code < 0)   return K_CLOUD;
    if (code <= 1)  return K_SUN;
    if (code == 2)  return K_PARTLY;
    if (code == 3)  return K_CLOUD;
    if (code <= 48) return K_FOG;
    if (code <= 67) return K_RAIN;
    if (code <= 77) return K_SNOW;
    if (code <= 82) return K_RAIN;
    if (code <= 86) return K_SNOW;
    return K_STORM;
}

static void blob(lv_obj_t *p, int x, int y, int w, int h, uint32_t col)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w < 1 ? 1 : w, h < 1 ? 1 : h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(col), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}

/* Cloud occupying w x (0.62 w) */
static void cloud(lv_obj_t *p, int x, int y, int w, uint32_t col)
{
    blob(p, x,                y + w * 24 / 100, w,           w * 38 / 100, col);
    blob(p, x + w * 14 / 100, y + w *  8 / 100, w * 42 / 100, w * 42 / 100, col);
    blob(p, x + w * 36 / 100, y,                w * 50 / 100, w * 50 / 100, col);
}

static void wx_icon(lv_obj_t *c, int S, int code)
{
    if (!c) return;
    lv_obj_clean(c);

    int t = S / 12 < 2 ? 2 : S / 12;     /* line / drop thickness */

    switch (weather_kind(code)) {
    case K_SUN:
        blob(c, S * 15 / 100, S * 15 / 100, S * 70 / 100, S * 70 / 100, 0xFFD60A);
        break;
    case K_PARTLY:
        blob(c, S * 38 / 100, 0, S * 55 / 100, S * 55 / 100, 0xFFD60A);
        cloud(c, 0, S * 32 / 100, S * 85 / 100, 0xF2F2F7);
        break;
    case K_FOG:
        cloud(c, S * 5 / 100, S * 2 / 100, S * 90 / 100, 0xAEB4BE);
        blob(c, S * 10 / 100, S * 66 / 100, S * 70 / 100, t, 0x8E96A3);
        blob(c, S * 28 / 100, S * 80 / 100, S * 62 / 100, t, 0x8E96A3);
        break;
    case K_RAIN:
        cloud(c, S * 5 / 100, 0, S * 90 / 100, 0xC9CED6);
        blob(c, S * 25 / 100, S * 68 / 100, t, S * 20 / 100, 0x64D2FF);
        blob(c, S * 47 / 100, S * 74 / 100, t, S * 20 / 100, 0x64D2FF);
        blob(c, S * 69 / 100, S * 68 / 100, t, S * 20 / 100, 0x64D2FF);
        break;
    case K_SNOW:
        cloud(c, S * 5 / 100, 0, S * 90 / 100, 0xC9CED6);
        blob(c, S * 25 / 100, S * 70 / 100, t + 1, t + 1, 0xFFFFFF);
        blob(c, S * 47 / 100, S * 80 / 100, t + 1, t + 1, 0xFFFFFF);
        blob(c, S * 69 / 100, S * 70 / 100, t + 1, t + 1, 0xFFFFFF);
        break;
    case K_STORM:
        cloud(c, S * 5 / 100, 0, S * 90 / 100, 0x9CA3AF);
        blob(c, S * 36 / 100, S * 66 / 100, t + 1, S * 24 / 100, 0xFFD60A);
        blob(c, S * 58 / 100, S * 72 / 100, t + 1, S * 24 / 100, 0xFFD60A);
        break;
    case K_CLOUD:
    default:
        cloud(c, 0, S * 18 / 100, S, 0xD1D5DB);
        break;
    }
}

static lv_obj_t *icon_box(lv_obj_t *parent, int S)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_set_size(c, S, S);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

/* ---------- networking (unchanged logic) ---------- */

static esp_err_t weather_http_event(esp_http_client_event_t *event)
{
    if (event->event_id == HTTP_EVENT_ON_DATA) {
        int remaining = sizeof(response_buffer) - response_length - 1;

        if (remaining > 0) {
            int copy_length = event->data_len;

            if (copy_length > remaining) {
                copy_length = remaining;
            }

            memcpy(response_buffer + response_length, event->data, copy_length);
            response_length += copy_length;
            response_buffer[response_length] = '\0';
        }
    }

    return ESP_OK;
}

static bool weather_fetch(void)
{
    response_length = 0;
    response_buffer[0] = '\0';

    esp_http_client_config_t config = {
        .url = WEATHER_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 8000,
        .event_handler = weather_http_event,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Weather request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status = esp_http_client_get_status_code(client);

    esp_http_client_cleanup(client);

    if (status != 200) {
        ESP_LOGW(TAG, "Weather server returned HTTP %d", status);
        return false;
    }

    return true;
}

/* ---------- UI update ---------- */

/* Date line and forecast day names. Needs the LVGL lock / LVGL thread. */
static void weather_apply_date(void)
{
    if (!body) return;

    if (!ui_time_synced()) {
        ui_set_text(date_lbl, "--");
        for (int i = 0; i < 3; i++) ui_set_text(fc_day[i], "---");
        return;
    }

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    char buf[24];
    strftime(buf, sizeof(buf), "%a %d %b", &tm);
    ui_set_text(date_lbl, buf);

    for (int i = 0; i < 3; i++) {
        ui_set_text(fc_day[i], dow_short[(tm.tm_wday + 1 + i) % 7]);
    }
}

/* Pushes `wx` into the widgets. Caller must hold the LVGL lock. */
static void weather_apply(void)
{
    if (!body) return;

    char buf[16];
    int code = (wx.state == WX_OK) ? wx.code : -1;

    lv_obj_set_style_bg_color(body, lv_color_hex(weather_bg(code)), 0);

    if (wx.state == WX_OK) {
        snprintf(buf, sizeof(buf), "%.0f", wx.temp);
        ui_set_text(temp_lbl, buf);
        ui_set_text(cond_lbl, weather_description(wx.code));
    } else {
        ui_set_text(temp_lbl, "--");
        ui_set_text(cond_lbl, wx.state == WX_LOADING ? "Fetching..."
                                                     : "Unavailable");
    }

    wx_icon(big_icon, ICON_SIZE, code);
    for (int i = 0; i < 3; i++) {
        wx_icon(fc_icon[i], FC_ICON, wx.state == WX_OK ? wx.fc_code[i] : -1);
    }

    weather_apply_date();
}

static void weather_publish(wx_state_t state)
{
    if (lvgl_port_lock(1000)) {
        wx.state = state;
        weather_apply();
        lvgl_port_unlock();
    }
}

static void weather_update(void)
{
    wx_state_t fallback = (wx.state == WX_OK) ? WX_OK : WX_ERROR;

    if (!weather_fetch()) {
        weather_publish(fallback);
        return;
    }

    cJSON *root = cJSON_Parse(response_buffer);

    if (root == NULL) {
        ESP_LOGW(TAG, "Invalid weather JSON");
        weather_publish(fallback);
        return;
    }

    cJSON *current = cJSON_GetObjectItem(root, "current");
    cJSON *daily   = cJSON_GetObjectItem(root, "daily");

    if (!cJSON_IsObject(current) || !cJSON_IsObject(daily)) {
        cJSON_Delete(root);
        weather_publish(fallback);
        return;
    }

    cJSON *temperature  = cJSON_GetObjectItem(current, "temperature_2m");
    cJSON *weather_code = cJSON_GetObjectItem(current, "weather_code");
    cJSON *daily_codes  = cJSON_GetObjectItem(daily, "weather_code");

    if (!cJSON_IsNumber(temperature) ||
        !cJSON_IsNumber(weather_code) ||
        !cJSON_IsArray(daily_codes) ||
        cJSON_GetArraySize(daily_codes) < 4) {
        cJSON_Delete(root);
        weather_publish(fallback);
        return;
    }

    int fc[3];
    for (int i = 0; i < 3; i++) {
        cJSON *it = cJSON_GetArrayItem(daily_codes, i + 1);   /* skip today */
        if (!cJSON_IsNumber(it)) {
            cJSON_Delete(root);
            weather_publish(fallback);
            return;
        }
        fc[i] = it->valueint;
    }

    if (lvgl_port_lock(1000)) {
        wx.temp  = (float)temperature->valuedouble;
        wx.code  = weather_code->valueint;
        for (int i = 0; i < 3; i++) wx.fc_code[i] = fc[i];
        wx.state = WX_OK;
        weather_apply();
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "Weather updated: %.1f C, code %d",
             temperature->valuedouble, weather_code->valueint);

    cJSON_Delete(root);
}

static void weather_task(void *arg)
{
    (void)arg;

    while (true) {
        weather_update();
        vTaskDelay(pdMS_TO_TICKS(WEATHER_UPDATE_MS));
    }
}

static void weather_icon(lv_obj_t *icon)
{
    ui_label(icon, &lv_font_montserrat_28, COL_TEXT, LV_SYMBOL_IMAGE,
             LV_ALIGN_CENTER, 0, 0);
}

/* Width of the circle's chord at distance dy from the center */
static int chord_w(int R, int dy)
{
    if (dy >= R) return 0;
    return (int)(2.0f * sqrtf((float)(R * R - dy * dy)));
}

static void weather_open(lv_obj_t *page)
{
    body = page;

    /* Vertical gradient: weather color on top, black at the bottom */
    lv_obj_set_style_bg_color(page, lv_color_hex(weather_bg(-1)), 0);
    lv_obj_set_style_bg_grad_color(page, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_dir(page, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_update_layout(page);
    int W = lv_obj_get_width(page);
    int H = lv_obj_get_height(page);
    int R = (W < H ? W : H) / 2;

    /* Big temperature + drawn degree ring (no degree glyph needed) */
    lv_obj_t *row = lv_obj_create(page);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, W * TEMP_X_PCT / 100,
                 H * TEMP_Y_PCT / 100);

    temp_lbl = ui_label(row, FONT_TEMP, lv_color_hex(WX_TXT), "--",
                        LV_ALIGN_DEFAULT, 0, 0);

    lv_obj_t *ring = lv_obj_create(row);
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, RING_SIZE, RING_SIZE);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(WX_TXT), 0);
    lv_obj_set_style_margin_left(ring, 2, 0);
    lv_obj_set_style_margin_top(ring, RING_TOP, 0);

    /* Weather icon, top right */
    big_icon = icon_box(page, ICON_SIZE);
    lv_obj_align(big_icon, LV_ALIGN_TOP_LEFT, W * ICON_X_PCT / 100,
                 H * ICON_Y_PCT / 100);

    /* City, condition, date */
    ui_label(page, &lv_font_montserrat_20, lv_color_hex(WX_TXT), WEATHER_CITY,
             LV_ALIGN_TOP_MID, 0, H * CITY_Y_PCT / 100);
    cond_lbl = ui_label(page, &lv_font_montserrat_14, lv_color_hex(WX_TXT),
                        "", LV_ALIGN_TOP_MID, 0, H * COND_Y_PCT / 100);
    date_lbl = ui_label(page, &lv_font_montserrat_14, lv_color_hex(WX_DIM),
                        "", LV_ALIGN_TOP_MID, 0, H * DATE_Y_PCT / 100);

    /* Forecast cards: row width limited by the circle at the lowest edge */
    int y0 = H * CARDS_Y_PCT / 100;
    int y1 = y0 + CARD_H;
    int d0 = abs(y0 - H / 2);
    int d1 = abs(y1 - H / 2);
    int dy = d0 > d1 ? d0 : d1;

    int rw = chord_w(R, dy) - EDGE_MARGIN * 2;
    if (rw > W - 24) rw = W - 24;
    if (rw < 120) rw = 120;

    int cw = (rw - CARD_GAP * 2) / 3;
    int x0 = (W - (cw * 3 + CARD_GAP * 2)) / 2;

    for (int i = 0; i < 3; i++) {
        lv_obj_t *card = lv_obj_create(page);
        lv_obj_remove_style_all(card);
        lv_obj_set_size(card, cw, CARD_H);
        lv_obj_set_pos(card, x0 + i * (cw + CARD_GAP), y0);
        lv_obj_set_style_radius(card, 14, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_20, 0);   /* frosted glass */
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        fc_day[i] = ui_label(card, &lv_font_montserrat_14,
                             lv_color_hex(WX_TXT), "---",
                             LV_ALIGN_TOP_MID, 0, 3);

        fc_icon[i] = icon_box(card, FC_ICON);
        lv_obj_align(fc_icon[i], LV_ALIGN_BOTTOM_MID, 0, -3);
    }

    if (weather_task_handle == NULL) {
        xTaskCreate(weather_task, "weather_task", 6144, NULL, 4,
                    &weather_task_handle);
    }

    weather_apply();
}

/* Called by the menu (LVGL context): keeps date and day names current */
static void weather_refresh(void)
{
    weather_apply_date();
}

static void weather_close(void)
{
    body = NULL;
    temp_lbl = cond_lbl = date_lbl = NULL;
    big_icon = NULL;
    for (int i = 0; i < 3; i++) fc_icon[i] = fc_day[i] = NULL;
}

const menu_app_t menu_weather_app = {
    .title = "Weather",
    .color = 0x64D2FF,
    .fullscreen = false,
    .build_icon = weather_icon,
    .open = weather_open,
    .refresh = weather_refresh,
    .close = weather_close,
};