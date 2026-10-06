#include "menu.h"
#include "ui_common.h"

static void phone_icon(lv_obj_t *icon)
{
    ui_label(icon, &lv_font_montserrat_28, COL_TEXT, LV_SYMBOL_CALL,
             LV_ALIGN_CENTER, 0, 0);
}

static void phone_open(lv_obj_t *page)
{
    lv_obj_t *body = ui_body_label(page);
    lv_label_set_text(body, "Not connected\nPair with a phone");
}

const menu_app_t menu_phone_app = {
    .title = "Phone",
    .color = 0x30D158,
    .build_icon = phone_icon,
    .open = phone_open,
};