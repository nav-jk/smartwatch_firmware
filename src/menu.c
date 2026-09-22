#include "../include/menu.h"
#include "../include/graphics.h"
#include "../include/icons.h"
#include "../include/framebuffer.h"
#include "../include/menu/fitness.h"

#define HIGHLIGHT_PAD   3
#define HIGHLIGHT_COLOR 0x07FF // cyan

static int selected_icon = 0;

void menu_init(void)
{
    selected_icon = 0;
}

void menu_navigate(void)
{
    selected_icon = (selected_icon + 1) % ICON_COUNT;
}

void menu_render(void)
{
    // draw_line(0, 50, 240, 50, 0xFFFF);
    // draw_line(0, 190, 240, 190, 0xFFFF);
    fb_clear(0x4529);
    for (int i = 0; i < ICON_COUNT; i++) {
        draw_icon(&icons[i]);
    }

    icon_t *sel = &icons[selected_icon];
    draw_rect_outline(
        sel->x - HIGHLIGHT_PAD,
        sel->y - HIGHLIGHT_PAD,
        sel->w + 2 * HIGHLIGHT_PAD,
        sel->h + 2 * HIGHLIGHT_PAD,
        HIGHLIGHT_COLOR
    );
}

void menu_select(void)
{
    switch (selected_icon) {

        case MENU_FITNESS:
            menu_fitness_init();
            break;

        // case MENU_PHONE:
        //     menu_phone_init();
        //     break;

        // case MENU_INTERNATIONAL_TIME:
        //     menu_intrn_time_init();
        //     break;

        // case MENU_WALLET:
        //     menu_wallet_init();
        //     break;

        // case MENU_CALENDAR:
        //     menu_calendar_init();
        //     break;

        // case MENU_WEATHER:
        //     menu_weather_init();
        //     break;

        // case MENU_WORKOUT:
        //     menu_workout_init();
        //     break;

        // case MENU_SETTINGS:
        //     menu_settings_init();
        //     break;

        default:
            break;
    }
}