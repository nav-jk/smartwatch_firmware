#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum {
    SETTINGS_DISPLAY,
    SETTINGS_BRIGHTNESS,
    SETTINGS_SOUND,
    SETTINGS_VIBRATION,
    SETTINGS_BLUETOOTH,
    SETTINGS_WIFI,
    SETTINGS_BATTERY,
    SETTINGS_ABOUT,

    SETTINGS_COUNT
} settings_item_t;

void menu_settings_init(void);
void settings_navigate(void);
void settings_select(void);

#endif