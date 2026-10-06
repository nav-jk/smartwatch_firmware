#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_crc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "nvs.h"

#include "cJSON.h"

#include "menu.h"
#include "ui_common.h"

#define WALLET_COLOR 0xFF9F0A

/* ---- configure these ---------------------------------------------- */

/* Direct link to your cards.json (must be HTTPS with a public CA cert). */
#define WALLET_URL \
    "https://nav-jk.github.io/card/EoeY13iwRKLGk8gDsiH7xWqRMJUvd4Bm/cards.json"
    

#define WALLET_AUTH ""

/* ------------------------------------------------------------------- */

#define MAX_CARDS     8
#define JSON_BUF_SIZE 8192
#define QR_SIZE       120

#define NVS_NS        "wallet"
#define NVS_KEY_JSON  "cards"
#define NVS_KEY_CRC   "crc"

static const char *TAG = "wallet";

typedef struct {
    char name[32];
    char sub[40];
    char code[256];
    uint32_t color;
} card_t;

typedef enum { FETCH_IDLE, FETCH_LOADING, FETCH_FAILED } fetch_state_t;

/* data (only touched while holding the LVGL lock, or from the open callback) */
static card_t cards[MAX_CARDS];
static int card_count;
static int cur;
static bool cache_loaded;
static volatile fetch_state_t fetch_state = FETCH_IDLE;

/* fetch scratch (used only by the fetch task) */
static char json_buf[JSON_BUF_SIZE];
static int json_len;
static card_t parsed[MAX_CARDS];

/* ui */
static lv_obj_t *wl_page;
static lv_obj_t *wl_qr;
static lv_obj_t *wl_name;
static lv_obj_t *wl_sub;
static lv_obj_t *wl_msg;
static lv_obj_t *wl_dot[MAX_CARDS];

static void wallet_icon(lv_obj_t *icon)
{
    ui_shape(icon, 32, 22, 5, COL_TEXT, 0, 2);
    ui_shape(icon, 32, 8, 4, lv_color_hex(0xFFE0B2), 0, -6);
    ui_shape(icon, 7, 7, LV_RADIUS_CIRCLE, lv_color_hex(WALLET_COLOR), 10, 4);
}

/* ------------------------------------------------------------------ */
/* JSON -> cards                                                       */
/* ------------------------------------------------------------------ */

static void copy_str(char *dst, size_t size, const cJSON *item)
{
    if (cJSON_IsString(item) && item->valuestring) {
        snprintf(dst, size, "%s", item->valuestring);
    } else {
        dst[0] = '\0';
    }
}

/*
 * Accepts either {"cards":[...]} or a bare [...] array.
 * Each card: name, sub (optional), code (required), color (optional "FF9F0A").
 * Returns the number of valid cards, or -1 if the JSON is unusable.
 */
static int parse_cards(const char *json, card_t *out, int max)
{
    cJSON *root = cJSON_Parse(json);

    if (root == NULL) {
        return -1;
    }

    cJSON *arr = cJSON_IsArray(root)
                     ? root
                     : cJSON_GetObjectItem(root, "cards");

    if (!cJSON_IsArray(arr)) {
        cJSON_Delete(root);
        return -1;
    }

    int n = 0;
    cJSON *item;

    cJSON_ArrayForEach(item, arr) {
        if (n >= max) {
            break;
        }

        card_t *c = &out[n];

        copy_str(c->code, sizeof(c->code), cJSON_GetObjectItem(item, "code"));

        if (c->code[0] == '\0') {
            continue;                       /* no payload, nothing to show */
        }

        copy_str(c->name, sizeof(c->name), cJSON_GetObjectItem(item, "name"));
        copy_str(c->sub, sizeof(c->sub), cJSON_GetObjectItem(item, "sub"));

        if (c->name[0] == '\0') {
            snprintf(c->name, sizeof(c->name), "Card %d", n + 1);
        }

        c->color = WALLET_COLOR;

        cJSON *col = cJSON_GetObjectItem(item, "color");

        if (cJSON_IsString(col) && col->valuestring[0]) {
            const char *s = col->valuestring;

            if (*s == '#') {
                s++;
            }

            c->color = (uint32_t)strtoul(s, NULL, 16) & 0xFFFFFF;
        }

        n++;
    }

    cJSON_Delete(root);
    return n;
}

/* ------------------------------------------------------------------ */
/* Offline cache in NVS                                                */
/* ------------------------------------------------------------------ */

static void cache_load(void)
{
    nvs_handle_t h;

    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return;
    }

    size_t len = sizeof(json_buf) - 1;

    /* json_buf is shared with the fetch task, so only load while idle */
    if (fetch_state != FETCH_LOADING &&
        nvs_get_blob(h, NVS_KEY_JSON, json_buf, &len) == ESP_OK) {

        json_buf[len] = '\0';

        int n = parse_cards(json_buf, parsed, MAX_CARDS);

        if (n >= 0) {
            memcpy(cards, parsed, sizeof(card_t) * n);
            card_count = n;
        }
    }

    nvs_close(h);
}

static void cache_save(void)
{
    nvs_handle_t h;

    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        return;
    }

    uint32_t crc = esp_crc32_le(0, (const uint8_t *)json_buf, json_len);
    uint32_t old = 0;

    /* skip the flash write if nothing changed */
    if (nvs_get_u32(h, NVS_KEY_CRC, &old) != ESP_OK || old != crc) {
        nvs_set_blob(h, NVS_KEY_JSON, json_buf, json_len);
        nvs_set_u32(h, NVS_KEY_CRC, crc);
        nvs_commit(h);
    }

    nvs_close(h);
}

/* ------------------------------------------------------------------ */
/* UI                                                                  */
/* ------------------------------------------------------------------ */

static void wallet_render(void)
{
    if (wl_page == NULL) {
        return;
    }

    if (card_count == 0) {
        lv_obj_add_flag(wl_qr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wl_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wl_sub, LV_OBJ_FLAG_HIDDEN);

        for (int i = 0; i < MAX_CARDS; i++) {
            lv_obj_add_flag(wl_dot[i], LV_OBJ_FLAG_HIDDEN);
        }

        if (fetch_state == FETCH_LOADING) {
            lv_label_set_text(wl_msg, "Loading...");
        } else if (fetch_state == FETCH_FAILED) {
            lv_label_set_text(wl_msg, "No cards\nCheck Wi-Fi");
        } else {
            lv_label_set_text(wl_msg, "No cards yet");
        }

        lv_obj_remove_flag(wl_msg, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (cur >= card_count) {
        cur = 0;
    }

    const card_t *c = &cards[cur];

    lv_obj_add_flag(wl_msg, LV_OBJ_FLAG_HIDDEN);

    /* QR */
    if (lv_qrcode_update(wl_qr, c->code, strlen(c->code)) == LV_RESULT_OK) {
        lv_obj_remove_flag(wl_qr, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(wl_qr, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(wl_msg, "Code too long");
        lv_obj_remove_flag(wl_msg, LV_OBJ_FLAG_HIDDEN);
    }

    /* text */
    lv_label_set_text(wl_name, c->name);
    lv_obj_remove_flag(wl_name, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(wl_sub, c->sub);
    lv_obj_set_style_text_color(wl_sub, lv_color_hex(c->color), 0);
    lv_obj_remove_flag(wl_sub, LV_OBJ_FLAG_HIDDEN);

    /* page dots, centred, active one in the card's colour */
    for (int i = 0; i < MAX_CARDS; i++) {
        if (i >= card_count || card_count < 2) {
            lv_obj_add_flag(wl_dot[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_align(wl_dot[i], LV_ALIGN_CENTER, (2 * i - (card_count - 1)) * 6, 94);
        lv_obj_set_style_bg_color(wl_dot[i],
                                  i == cur ? lv_color_hex(c->color)
                                           : lv_color_hex(0x2C2C2E), 0);
        lv_obj_remove_flag(wl_dot[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/* ------------------------------------------------------------------ */
/* Network                                                             */
/* ------------------------------------------------------------------ */

static esp_err_t wallet_http_event(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA) {
        return ESP_OK;
    }

    int remaining = sizeof(json_buf) - json_len - 1;

    if (remaining <= 0) {
        return ESP_OK;
    }

    int len = event->data_len;

    if (len > remaining) {
        len = remaining;
    }

    memcpy(json_buf + json_len, event->data, len);
    json_len += len;
    json_buf[json_len] = '\0';

    return ESP_OK;
}

static bool fetch_cards_json(void)
{
    json_len = 0;
    json_buf[0] = '\0';

    esp_http_client_config_t config = {
        .url = WALLET_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .event_handler = wallet_http_event,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_agent = "Smartwatch-Wallet",
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        return false;
    }

    if (WALLET_AUTH[0] != '\0') {
        esp_http_client_set_header(client, "Authorization", WALLET_AUTH);
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);

    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Request failed: %s", esp_err_to_name(err));
        return false;
    }

    if (status != 200) {
        ESP_LOGW(TAG, "Server returned HTTP %d", status);
        return false;
    }

    return json_len > 0;
}

static void wallet_fetch_task(void *arg)
{
    (void)arg;

    bool ok = fetch_cards_json();
    int n = ok ? parse_cards(json_buf, parsed, MAX_CARDS) : -1;

    if (n >= 0) {
        cache_save();
    }

    if (lvgl_port_lock(2000)) {
        if (n >= 0) {
            memcpy(cards, parsed, sizeof(card_t) * n);
            card_count = n;
            fetch_state = FETCH_IDLE;
        } else {
            ESP_LOGW(TAG, "Wallet sync failed");
            fetch_state = FETCH_FAILED;
        }

        wallet_render();
        lvgl_port_unlock();
    } else {
        fetch_state = n >= 0 ? FETCH_IDLE : FETCH_FAILED;
    }

    vTaskDelete(NULL);
}

static void wallet_sync(void)
{
    if (fetch_state == FETCH_LOADING) {
        return;
    }

    fetch_state = FETCH_LOADING;

    if (xTaskCreate(wallet_fetch_task, "wallet_fetch", 8192, NULL, 4, NULL) != pdPASS) {
        fetch_state = FETCH_FAILED;
    }
}

/* ------------------------------------------------------------------ */
/* App callbacks                                                       */
/* ------------------------------------------------------------------ */

static void wallet_clicked(lv_event_t *e)
{
    (void)e;

    if (card_count > 1) {
        cur = (cur + 1) % card_count;
        wallet_render();
    }
}

static void wallet_long_pressed(lv_event_t *e)
{
    (void)e;

    wallet_sync();
    wallet_render();
}

static void wallet_open(lv_obj_t *page)
{
    wl_page = page;

    if (!cache_loaded) {
        cache_loaded = true;
        cache_load();
    }

    /* QR with a white quiet-zone border so phones can scan it */
    wl_qr = lv_qrcode_create(page);
    lv_qrcode_set_size(wl_qr, QR_SIZE);
    lv_qrcode_set_dark_color(wl_qr, lv_color_black());
    lv_qrcode_set_light_color(wl_qr, lv_color_white());
    lv_obj_set_style_border_color(wl_qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(wl_qr, 5, 0);
    lv_obj_align(wl_qr, LV_ALIGN_CENTER, 0, -22);
    lv_obj_remove_flag(wl_qr, LV_OBJ_FLAG_CLICKABLE);

    wl_name = ui_label(page, &lv_font_montserrat_20, COL_TEXT, "",
                       LV_ALIGN_CENTER, 0, 56);
    lv_obj_set_width(wl_name, 150);
    lv_label_set_long_mode(wl_name, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(wl_name, LV_TEXT_ALIGN_CENTER, 0);

    wl_sub = ui_label(page, &lv_font_montserrat_14, COL_MUTED, "",
                      LV_ALIGN_CENTER, 0, 76);
    lv_obj_set_width(wl_sub, 130);
    lv_label_set_long_mode(wl_sub, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(wl_sub, LV_TEXT_ALIGN_CENTER, 0);

    wl_msg = ui_label(page, &lv_font_montserrat_20, COL_TEXT, "",
                      LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(wl_msg, 180);
    lv_obj_set_style_text_align(wl_msg, LV_TEXT_ALIGN_CENTER, 0);

    for (int i = 0; i < MAX_CARDS; i++) {
        lv_obj_t *d = ui_shape(page, 6, 6, LV_RADIUS_CIRCLE,
                               lv_color_hex(0x2C2C2E), 0, 94);
        lv_obj_add_flag(d, LV_OBJ_FLAG_HIDDEN);
        wl_dot[i] = d;
    }

    /* tap = next card, long press = re-sync */
    lv_obj_add_flag(page, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(page, wallet_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(page, wallet_long_pressed, LV_EVENT_LONG_PRESSED, NULL);

    wallet_render();
    wallet_sync();
}

static void wallet_close(void)
{
    wl_page = NULL;
    wl_qr = NULL;
    wl_name = NULL;
    wl_sub = NULL;
    wl_msg = NULL;

    for (int i = 0; i < MAX_CARDS; i++) {
        wl_dot[i] = NULL;
    }
}

const menu_app_t menu_wallet_app = {
    .title = "Wallet",
    .color = WALLET_COLOR,
    .build_icon = wallet_icon,
    .open = wallet_open,
    .close = wallet_close,
};