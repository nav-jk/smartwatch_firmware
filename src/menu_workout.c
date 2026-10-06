#include "menu.h"
#include "ui_common.h"

static void workout_icon(lv_obj_t *icon)
{
    /* dumbbell */
    ui_shape(icon, 26, 4, 2, COL_DARK, 0, 0);
    ui_shape(icon, 6, 20, 3, COL_DARK, -13, 0);
    ui_shape(icon, 6, 20, 3, COL_DARK, 13, 0);
    ui_shape(icon, 4, 12, 2, COL_DARK, -19, 0);
    ui_shape(icon, 4, 12, 2, COL_DARK, 19, 0);
}

static void workout_open(lv_obj_t *page)
{
    lv_obj_t *body = ui_body_label(page);
    lv_label_set_text(body, "Ready\nComing soon");
}

const menu_app_t menu_workout_app = {
    .title = "Workout",
    .color = 0xFFD60A,
    .build_icon = workout_icon,
    .open = workout_open,
};