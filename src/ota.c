#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"

#include "cJSON.h"

#include "ota.h"
#include "settings_update.h"
#include "ui_common.h"

#define GITHUB_API_URL \
    "https://api.github.com/repos/nav-jk/smartwatch_firmware/releases/latest"

#define GITHUB_DOWNLOAD_URL \
    "https://github.com/nav-jk/smartwatch_firmware/releases/download/%s/smartwatch_firmware.bin"

#define OTA_ASSET_NAME "smartwatch_firmware.bin"

static const char *TAG = "ota";

static volatile bool running;

static char response[16384];
static int response_length;

static char latest_version[32];
static char firmware_url[256];

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

/* Show a failure on the cloud screen (it returns to Settings by itself). */
static void ota_fail(const char *msg)
{
    settings_update_done(false, msg);

    running = false;
    vTaskDelete(NULL);
}

static void ota_task(void *arg)
{
    (void)arg;

    running = true;

    settings_update_open("Checking...");
    settings_update_progress(-1);           /* spinning ring */

    const esp_app_desc_t *app =
        esp_app_get_description();

    if (app == NULL) {
        ota_fail("Version error");
        return;
    }

    ESP_LOGI(
        TAG,
        "Current version: %s",
        app->version
    );

    if (!get_latest_release()) {
        ota_fail("Check failed");
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

        settings_update_done(true, "Up to date");

        running = false;
        vTaskDelete(NULL);
        return;
    }

    settings_update_status("Update found", latest_version);

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_http_client_config_t http_config = {
        .url = firmware_url,
        .timeout_ms = 30000,
        .buffer_size = 8192,
        .buffer_size_tx = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_agent = "Smartwatch-OTA",
        .disable_auto_redirect = false,
        .max_redirection_count = 10,
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

        ota_fail("OTA failed");
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

        ota_fail("Invalid firmware");
        return;
    }

    int image_size =
        esp_https_ota_get_image_size(handle);

    char detail[48];
    int last_progress = -1;

    settings_update_status("Downloading", latest_version);
    settings_update_progress(0);

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

            /* only touch the UI when the number actually changes */
            if (progress != last_progress) {
                last_progress = progress;

                snprintf(
                    detail,
                    sizeof(detail),
                    "%s  %d%%",
                    latest_version,
                    progress
                );

                settings_update_progress(progress);
                settings_update_status(NULL, detail);
            }
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

        ota_fail("Download failed");
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

        ota_fail("Update failed");
        return;
    }

    settings_update_progress(100);
    settings_update_done(true, "Restarting...");

    vTaskDelay(pdMS_TO_TICKS(1500));

    esp_restart();
}

/*
 * Called by Settings when it opens. The old on-page status label and
 * progress bar are gone: the cloud update screen now shows all of that.
 * Behaviour is unchanged otherwise: opening Settings starts a check.
 */
void ota_bind_ui(lv_obj_t *page)
{
    (void)page;

    ota_check();
}

void ota_unbind_ui(void)
{
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