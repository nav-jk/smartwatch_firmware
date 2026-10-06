#include "menu.h"
#include "ui_common.h"

#define WALLET_COLOR 0xFF9F0A

static void wallet_icon(lv_obj_t *icon)
{
    ui_shape(icon, 32, 22, 5, COL_TEXT, 0, 2);
    ui_shape(icon, 32, 8, 4, lv_color_hex(0xFFE0B2), 0, -6);
    ui_shape(icon, 7, 7, LV_RADIUS_CIRCLE, lv_color_hex(WALLET_COLOR), 10, 4);
}

static void wallet_open(lv_obj_t *page)
{
    lv_obj_t *body = ui_body_label(page);
    lv_label_set_text(body, "No cards yet");
}

const menu_app_t menu_wallet_app = {
    .title = "Wallet",
    .color = WALLET_COLOR,
    .build_icon = wallet_icon,
    .open = wallet_open,
};