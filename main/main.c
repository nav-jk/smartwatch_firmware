#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_gc9a01.h"
#include "../include/clock_module.h"
#include "../include/framebuffer.h"

#include "../include/specs.h"
#include "../include/menu.h"

static const char *TAG = "gc9a01_demo";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;

static void lcd_init(void)
{
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(
            LCD_SPI_HOST,
            &buscfg,
            SPI_DMA_CH_AUTO
        )
    );

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_spi(
            (esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
            &io_config,
            &io_handle
        )
    );

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_gc9a01(
            io_handle,
            &panel_config,
            &panel_handle
        )
    );

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

void app_main(void)
{
    lcd_init();

    if (!fb_init()) {
        return;
    }

    circle c = {
        .cen = {
            .cx = LCD_H_RES / 2,
            .cy = LCD_V_RES / 2
        },
        .radius = 8,
        .color = COLOR_CAP
    };

    circle rim = {
        .cen = { .cx = LCD_H_RES / 2, .cy = LCD_V_RES / 2 },
        .radius = 119,
        .color = 0x0000
    };

    wall_clock cur_clock = {
        .seconds = 0,
        .minutes = 0,
        .hour = 0
    };

    TickType_t last_wake = xTaskGetTickCount();

    menu_init();

    while (1) {
        // fb_copy_wallpaper();

        // draw_ring(&rim, 4);
        // draw_dial_ticks();
        // draw_clock(&cur_clock);
        // draw_circle(&c);

        // fb_flush(panel_handle);

        // clock_advance(&cur_clock);

        // vTaskDelayUntil(
        //     &last_wake,
        //     pdMS_TO_TICKS(1000)
        // );

        fb_clear(0x0000);
        menu_render();
        vTaskDelay(pdMS_TO_TICKS(1000));
        // menu_navigate();
        // fb_flush(panel_handle);

        while(1){
            fb_clear(0x0000);
            menu_select();
            fb_flush(panel_handle);
            vTaskDelay(pdMS_TO_TICKS(10000));
        }

        

    }
}