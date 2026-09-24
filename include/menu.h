#ifndef MENU_H
#define MENU_H


#include "menu/fitness.h"
#include "menu/phone.h"
#include "menu/international_time.h"
#include "menu/wallet.h"
#include "menu/calendar.h"
#include "menu/weather.h"
#include "menu/workout.h"
#include "menu/settings.h"

void menu_init(void);    
void menu_render(void);   
void menu_navigate(void);
void menu_select(void); 
void menu_app_navigate(void); // forwards to the active app's navigate function, if it has one
void menu_app_select(void);   // forwards to the active app's select function, if it has one

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

#endif