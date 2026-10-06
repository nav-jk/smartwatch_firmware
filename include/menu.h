#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "lvgl.h"

/* Order = reading order in the honeycomb launcher */
typedef enum {
    MENU_FITNESS,
    MENU_PHONE,
    MENU_INTERNATIONAL_TIME,
    MENU_WALLET,
    MENU_CALENDAR,
    MENU_WEATHER,
    MENU_WORKOUT,
    MENU_SETTINGS,
    MENU_COUNT
} menu_item_t;

/*
 * One menu app = one src/menu/menu_<name>.c file exporting one of these.
 * Every hook except `title`, `color` and `build_icon` is optional (NULL).
 */
typedef struct {
    const char *title;
    uint32_t color;          /* icon background / accent colour (0xRRGGBB) */
    bool fullscreen;         /* true: the app draws its own header, no title bar */

    /* Draw the glyph inside the round launcher icon */
    void (*build_icon)(lv_obj_t *icon);

    /* Build the app page inside `page` (full 240x240 container, centre origin) */
    void (*open)(lv_obj_t *page);

    /* Called once per second while the page is open */
    void (*refresh)(void);

    /* Called when the page is closed (its widgets are deleted afterwards) */
    void (*close)(void);

    /* Called once per minute, even when the app is closed (e.g. calendar icon) */
    void (*on_minute)(const struct tm *tm, bool synced);
} menu_app_t;

extern const menu_app_t menu_fitness_app;
extern const menu_app_t menu_phone_app;
extern const menu_app_t menu_world_time_app;
extern const menu_app_t menu_wallet_app;
extern const menu_app_t menu_calendar_app;
extern const menu_app_t menu_weather_app;
extern const menu_app_t menu_workout_app;
extern const menu_app_t menu_settings_app;

#endif