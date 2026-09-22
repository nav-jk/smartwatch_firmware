#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_lcd_panel_ops.h"

// allocates the DMA-capable framebuffer; returns false on failure
bool fb_init(void);

void fb_set(int x, int y, uint16_t color);
void fb_clear(uint16_t color);
void fb_copy_wallpaper(void);

// blits the current framebuffer to the panel
void fb_flush(esp_lcd_panel_handle_t panel_handle);

#endif // FRAMEBUFFER_H