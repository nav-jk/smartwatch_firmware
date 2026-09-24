#ifndef WEATHER_H
#define WEATHER_H

typedef struct {
    int temperature;
    int humidity;
    int wind;
} weather_data_t;

weather_data_t weather_fetch_data(void);
void menu_weather_init(void);

#endif