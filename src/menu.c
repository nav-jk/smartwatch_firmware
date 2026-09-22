#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_gc9a01.h"

#include "../include/menu.h"
#include "../include/specs.h"
#include "../include/graphics.h"

void menu_init(void){
    draw_line(0,50,240,50,0xFFFF);
    draw_line(0,190,240,190,0xFFFF);
    draw_rect(30, 60, 40, 40, 0xFFFF); 
}
