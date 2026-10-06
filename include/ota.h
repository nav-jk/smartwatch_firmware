#ifndef OTA_H
#define OTA_H

#include <stdbool.h>
#include "lvgl.h"

void ota_bind_ui(lv_obj_t *page);
void ota_unbind_ui(void);
void ota_check(void);
bool ota_is_running(void);

#endif