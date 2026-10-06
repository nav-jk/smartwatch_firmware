#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_system.h"

#include "cJSON.h"

#include "ota.h"
#include "ui_common.h"

#define GITHUB_API_URL \
    "https://api.github.com/repos/nav-jk/smartwatch_firmware/releases/latest"

#define GITHUB_DOWNLOAD_URL \
    "https://github.com/nav-jk/smartwatch_firmware/releases/download/%s/smartwatch_firmware.bin"

#define OTA_ASSET_NAME "smartwatch_firmware.bin"

static const char *TAG = "ota";

static bool running;

static lv_obj_t *status_label;
static lv_obj_t *progress_bar;

static char response[16384];
static int response_length;

static char latest_version[32];
static char firmware_url[256];

static void ota_set_status(const char *text)
{
    if (status_label == NULL) {
        return;
    }

    if (lvgl_port_lock(1000)) {
        ui_set_text(status_label, text);
        lvgl_port_unlock();
    }
}

static void ota_set_progress(int value)
{
    if (progress_bar == NULL) {
        return;
    }

    if (lvgl_port_lock(1000)) {
        lv_bar_set_value(progress_bar, value, LV_ANIM_OFF);
        lvgl_port_unlock();
    }
}

static esp_err_t github_event_handler(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA) {
        return ESP_OK;
    }

    int remaining =
        sizeof(response) - response_length - 1;

    if (remaining <= 0) {
        return ESP_OK;
    }

    int len = event->data_len;

    if (len > remaining) {
        len = remaining;
    }

    memcpy(
        response + response_length,
        event->data,
        len
    );

    response_length += len;
    response[response_length] = '\0';

    return ESP_OK;
}

static int version_number(const char **ptr)
{
    while (**ptr == 'v' || **ptr == 'V') {
        (*ptr)++;
    }

    return strtol(*ptr, (char **)ptr, 10);
}

static int version_compare(
    const char *a,
    const char *b
)
{
    const char *pa = a;
    const char *pb = b;

    for (int i = 0; i < 3; i++) {
        int va = version_number(&pa);
        int vb = version_number(&pb);

        if (va > vb) {
            return 1;
        }

        if (va < vb) {
            return -1;
        }

        if (*pa == '.') {
            pa++;
        }

        if (*pb == '.') {
            pb++;
        }
    }

    return 0;
}

static bool get_latest_release(void)
{
    response_length = 0;
    response[0] = '\0';

    esp_http_client_config_t config = {
        .url = GITHUB_API_URL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .event_handler = github_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_agent = "Smartwatch-OTA",
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP client init failed");
        return false;
    }

    esp_err_t err =
        esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "GitHub request failed: %s",
            esp_err_to_name(err)
        );

        esp_http_client_cleanup(client);
        return false;
    }

    int status =
        esp_http_client_get_status_code(client);

    esp_http_client_cleanup(client);

    if (status != 200) {
        ESP_LOGW(
            TAG,
            "GitHub returned HTTP %d",
            status
        );

        return false;
    }

    cJSON *root = cJSON_Parse(response);

    if (root == NULL) {
        ESP_LOGW(TAG, "Invalid GitHub response");
        return false;
    }

    cJSON *tag =
        cJSON_GetObjectItem(root, "tag_name");

    cJSON *assets =
        cJSON_GetObjectItem(root, "assets");

    if (!cJSON_IsString(tag) ||
        !cJSON_IsArray(assets)) {

        cJSON_Delete(root);
        return false;
    }

    strncpy(
        latest_version,
        tag->valuestring,
        sizeof(latest_version) - 1
    );

    latest_version[
        sizeof(latest_version) - 1
    ] = '\0';

    bool asset_found = false;

    cJSON *asset;

    cJSON_ArrayForEach(asset, assets) {
        cJSON *name =
            cJSON_GetObjectItem(asset, "name");

        if (cJSON_IsString(name) &&
            strcmp(
                name->valuestring,
                OTA_ASSET_NAME
            ) == 0) {

            asset_found = true;
            break;
        }
    }

    if (asset_found) {
        snprintf(
            firmware_url,
            sizeof(firmware_url),
            GITHUB_DOWNLOAD_URL,
            latest_version
        );
    }

    cJSON_Delete(root);

    return asset_found;
}

static void ota_task(void *arg)
{
    (void)arg;

    running = true;

    ota_set_status("Checking...");
    ota_set_progress(0);

    const esp_app_desc_t *app =
        esp_app_get_description();

    if (app == NULL) {
        ota_set_status("Version error");
        running = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(
        TAG,
        "Current version: %s",
        app->version
    );

    if (!get_latest_release()) {
        ota_set_status("Update check failed");
        running = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(
        TAG,
        "Latest version: %s",
        latest_version
    );

    if (version_compare(
            latest_version,
            app->version
        ) <= 0) {

        ota_set_progress(100);
        ota_set_status("Up to date");

        running = false;
        vTaskDelete(NULL);
        return;
    }

    char status[64];

    snprintf(
        status,
        sizeof(status),
        "Update available\n%s",
        latest_version
    );

    ota_set_status(status);

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_http_client_config_t http_config = {
        .url = firmware_url,
        .timeout_ms = 15000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_agent = "Smartwatch-OTA",
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t handle = NULL;

    esp_err_t err =
        esp_https_ota_begin(
            &ota_config,
            &handle
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OTA begin failed: %s",
            esp_err_to_name(err)
        );

        ota_set_status("OTA failed");
        running = false;
        vTaskDelete(NULL);
        return;
    }

    esp_app_desc_t new_app;

    err = esp_https_ota_get_img_desc(
        handle,
        &new_app
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Invalid OTA image");

        esp_https_ota_abort(handle);

        ota_set_status("Invalid firmware");
        running = false;
        vTaskDelete(NULL);
        return;
    }

    int image_size =
        esp_https_ota_get_image_size(handle);

    while (true) {
        err = esp_https_ota_perform(handle);

        int downloaded =
            esp_https_ota_get_image_len_read(handle);

        if (image_size > 0) {
            int progress =
                downloaded * 100 / image_size;

            if (progress > 100) {
                progress = 100;
            }

            ota_set_progress(progress);

            snprintf(
                status,
                sizeof(status),
                "Downloading %d%%",
                progress
            );

            ota_set_status(status);
        }

        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        break;
    }

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OTA download failed: %s",
            esp_err_to_name(err)
        );

        esp_https_ota_abort(handle);

        ota_set_status("Download failed");
        running = false;
        vTaskDelete(NULL);
        return;
    }

    err =
        esp_https_ota_finish(handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "OTA finish failed: %s",
            esp_err_to_name(err)
        );

        ota_set_status("Update failed");
        running = false;
        vTaskDelete(NULL);
        return;
    }

    ota_set_progress(100);
    ota_set_status("Update complete");

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_restart();
}

void ota_bind_ui(lv_obj_t *page)
{
    status_label = ui_label(
        page,
        &lv_font_montserrat_14,
        COL_MUTED,
        "Checking...",
        LV_ALIGN_CENTER,
        0,
        80
    );

    lv_obj_set_width(status_label, 180);
    lv_obj_set_style_text_align(
        status_label,
        LV_TEXT_ALIGN_CENTER,
        0
    );

    progress_bar = lv_bar_create(page);

    lv_obj_set_size(
        progress_bar,
        150,
        8
    );

    lv_obj_align(
        progress_bar,
        LV_ALIGN_CENTER,
        0,
        102
    );

    lv_obj_set_style_bg_color(
        progress_bar,
        COL_TRACK,
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_opa(
        progress_bar,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    lv_obj_set_style_bg_color(
        progress_bar,
        COL_ACCENT,
        LV_PART_INDICATOR
    );

    lv_obj_set_style_bg_opa(
        progress_bar,
        LV_OPA_COVER,
        LV_PART_INDICATOR
    );

    lv_bar_set_range(
        progress_bar,
        0,
        100
    );

    lv_bar_set_value(
        progress_bar,
        0,
        LV_ANIM_OFF
    );

    ota_check();
}

void ota_unbind_ui(void)
{
    status_label = NULL;
    progress_bar = NULL;
}

void ota_check(void)
{
    if (running) {
        return;
    }

    xTaskCreate(
        ota_task,
        "ota_task",
        8192,
        NULL,
        5,
        NULL
    );
}

bool ota_is_running(void)
{
    return running;
}