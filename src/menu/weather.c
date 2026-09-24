#include <stdio.h>

#include "../include/menu/weather.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

weather_data_t weather_fetch_data(void)
{
    weather_data_t weather = {
        .temperature = 28,
        .humidity = 72,
        .wind = 12
    };

    return weather;
}

void menu_weather_init(void)
{
    fb_clear(0x0000);

    weather_data_t weather = weather_fetch_data();

    char temp_text[10];
    char humidity_text[15];
    char wind_text[15];

    snprintf(
        temp_text,
        sizeof(temp_text),
        "%d C",
        weather.temperature
    );

    snprintf(
        humidity_text,
        sizeof(humidity_text),
        "HUM %d%%",
        weather.humidity
    );

    snprintf(
        wind_text,
        sizeof(wind_text),
        "WIND %d",
        weather.wind
    );

    // Location
    draw_text_box(
        75, 12,
        110, 22,
        "BENGALURU",
        0xFFFF,
        1
    );

    // Sun
    circle sun = {
        .cen = {120, 70},
        .radius = 25,
        .color = 0xFFE0
    };

    draw_circle(&sun);

    // Sun rays
    draw_line(120, 34, 120, 25, 0xFFE0);
    draw_line(120, 106, 120, 115, 0xFFE0);
    draw_line(84, 70, 75, 70, 0xFFE0);
    draw_line(156, 70, 165, 70, 0xFFE0);

    draw_line(95, 45, 88, 38, 0xFFE0);
    draw_line(145, 45, 152, 38, 0xFFE0);
    draw_line(95, 95, 88, 102, 0xFFE0);
    draw_line(145, 95, 152, 102, 0xFFE0);

    // Temperature
    draw_text_box(
        65, 112,
        110, 35,
        temp_text,
        0xFFFF,
        3
    );

    // Condition
    draw_text_box(
        70, 145,
        100, 20,
        "SUNNY",
        0xFFE0,
        1
    );

    // Separator
    draw_line(
        35, 172,
        205, 172,
        0x7BEF
    );

    // Humidity
    draw_text_box(
        35, 183,
        90, 20,
        humidity_text,
        0x07FF,
        1
    );

    // Wind
    draw_text_box(
        135, 183,
        90, 20,
        wind_text,
        0xFFFF,
        1
    );
}