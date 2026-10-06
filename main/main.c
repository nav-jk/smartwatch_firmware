#include "esp_err.h"
#include "esp_log.h"

#include "display.h"
#include "network.h"
#include "watch_ui.h"

static const char *TAG = "watch";

void app_main(void)
{
    ESP_ERROR_CHECK(display_init());
    watch_ui_init();
    wifi_time_init();

    ESP_LOGI(TAG, "Watch started");
}