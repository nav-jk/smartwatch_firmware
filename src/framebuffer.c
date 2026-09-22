#include "esp_heap_caps.h"
#include "esp_log.h"

#include "../include/framebuffer.h"
#include "../include/clock.h"
#include "../include/wallpaper.h"

static const char *TAG = "framebuffer";

static uint16_t *fb = NULL;

bool fb_init(void)
{
    fb = heap_caps_malloc(
        LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
        MALLOC_CAP_DMA
    );

    if (fb == NULL) {
        ESP_LOGE(TAG, "Framebuffer allocation failed!");
        return false;
    }

    return true;
}

void fb_set(int x, int y, uint16_t color)
{
    if (x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES)
        return;

    fb[y * LCD_H_RES + x] =
        (color >> 8) | (color << 8);
}

void fb_clear(uint16_t color)
{
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++)
        fb[i] = color;
}

void fb_copy_wallpaper(void)
{
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) {
        uint16_t pixel = wallpaper[i];

        fb[i] = (pixel >> 8) | (pixel << 8);
    }
}

void fb_flush(esp_lcd_panel_handle_t panel_handle)
{
    esp_lcd_panel_draw_bitmap(
        panel_handle,
        0,
        0,
        LCD_H_RES,
        LCD_V_RES,
        fb
    );
}