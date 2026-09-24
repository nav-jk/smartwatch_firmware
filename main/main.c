#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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

#define BUTTON_GPIO GPIO_NUM_18

#define BUTTON_DEBOUNCE_MS        30
#define BUTTON_LONG_PRESS_MS      800
#define BUTTON_VERY_LONG_PRESS_MS 2500

#define CLOCK_TICK_MS     1000
#define POLL_INTERVAL_MS  20

typedef enum {
    APP_STATE_CLOCK,
    APP_STATE_MENU,
    APP_STATE_APP
} app_state_t;

typedef enum {
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_VERY_LONG_PRESS
} button_event_t;

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static app_state_t current_state = APP_STATE_CLOCK;

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
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

static void button_init(void)
{
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));
}

static button_event_t poll_button(void)
{
    static bool was_pressed = false;
    static TickType_t press_start = 0;

    bool pressed = (gpio_get_level(BUTTON_GPIO) == 0);

    if (pressed && !was_pressed) {
        press_start = xTaskGetTickCount();
        was_pressed = true;
        return BUTTON_EVENT_NONE;
    }

    if (!pressed && was_pressed) {
        was_pressed = false;

        uint32_t duration_ms = pdTICKS_TO_MS(xTaskGetTickCount() - press_start);

        if (duration_ms < BUTTON_DEBOUNCE_MS) {
            return BUTTON_EVENT_NONE;
        }
        if (duration_ms >= BUTTON_VERY_LONG_PRESS_MS) {
            return BUTTON_EVENT_VERY_LONG_PRESS;
        }
        if (duration_ms >= BUTTON_LONG_PRESS_MS) {
            return BUTTON_EVENT_LONG_PRESS;
        }
        return BUTTON_EVENT_SHORT_PRESS;
    }

    return BUTTON_EVENT_NONE;
}

static bool handle_navigation(button_event_t event)
{
    if (event == BUTTON_EVENT_NONE) {
        return false;
    }

    switch (current_state) {
        case APP_STATE_CLOCK:
            if (event == BUTTON_EVENT_LONG_PRESS || event == BUTTON_EVENT_VERY_LONG_PRESS) {
                menu_init();
                current_state = APP_STATE_MENU;
                return true;
            }
            break;

        case APP_STATE_MENU:
            if (event == BUTTON_EVENT_SHORT_PRESS) {
                menu_navigate();
                return true;
            }
            if (event == BUTTON_EVENT_LONG_PRESS) {
                current_state = APP_STATE_APP;
                return true;
            }
            if (event == BUTTON_EVENT_VERY_LONG_PRESS) {
                current_state = APP_STATE_CLOCK;
                return true;
            }
            break;

        case APP_STATE_APP:
            if (event == BUTTON_EVENT_SHORT_PRESS) {
                menu_app_navigate();
                return true;
            }
            if (event == BUTTON_EVENT_LONG_PRESS) {
                menu_app_select();
                return true;
            }
            if (event == BUTTON_EVENT_VERY_LONG_PRESS) {
                current_state = APP_STATE_MENU;
                return true;
            }
            break;
    }

    return false;
}

static void draw_clock_screen(wall_clock *clock)
{
    circle rim = {
        .cen = { .cx = LCD_H_RES / 2, .cy = LCD_V_RES / 2 },
        .radius = 119,
        .color = 0x0000
    };
    circle c = {
        .cen = { .cx = LCD_H_RES / 2, .cy = LCD_V_RES / 2 },
        .radius = 8,
        .color = COLOR_CAP
    };

    fb_copy_wallpaper();
    draw_ring(&rim, 4);
    draw_dial_ticks();
    draw_clock(clock);
    draw_circle(&c);
    fb_flush(panel_handle);
}

static void draw_menu_screen(void)
{
    fb_clear(0x0000);
    menu_render();
    fb_flush(panel_handle);
}

static void draw_app_screen(void)
{
    fb_clear(0x0000);
    menu_select();
    fb_flush(panel_handle);
}

static void render_current_screen(wall_clock *clock)
{
    switch (current_state) {
        case APP_STATE_CLOCK: draw_clock_screen(clock); break;
        case APP_STATE_MENU:  draw_menu_screen();       break;
        case APP_STATE_APP:   draw_app_screen();        break;
    }
}

static void display_task(void *arg)
{
    wall_clock cur_clock = { .seconds = 30, .minutes = 48, .hour = 11 };

    TickType_t last_clock_update = xTaskGetTickCount();
    bool screen_needs_update = true;

    while (1) {
        button_event_t event = poll_button();

        if (handle_navigation(event)) {
            screen_needs_update = true;
        }

        TickType_t now = xTaskGetTickCount();
        if (now - last_clock_update >= pdMS_TO_TICKS(CLOCK_TICK_MS)) {
            clock_advance(&cur_clock);
            last_clock_update = now;

            if (current_state == APP_STATE_CLOCK) {
                screen_needs_update = true;
            }
        }

        if (screen_needs_update) {
            render_current_screen(&cur_clock);
            screen_needs_update = false;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}

void app_main(void)
{
    lcd_init();

    if (!fb_init()) {
        return;
    }

    button_init();
    menu_init();

    xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}