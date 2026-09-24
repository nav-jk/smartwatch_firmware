#include <stdio.h>

#include "../include/menu/calendar.h"
#include "../include/graphics.h"
#include "../include/framebuffer.h"
#include "../include/text.h"

calendar_date_t calendar_fetch_date(void)
{
    calendar_date_t date = {
        .day = 22,
        .month = 9,
        .year = 2026
    };

    return date;
}

void menu_calendar_init(void)
{
    fb_clear(0x0000);

    calendar_date_t date = calendar_fetch_date();

    char date_text[20];
    char year_text[10];

    snprintf(
        date_text,
        sizeof(date_text),
        "%02d",
        date.day
    );

    snprintf(
        year_text,
        sizeof(year_text),
        "%04d",
        date.year
    );

    // Month and year
    draw_text_box(
        65, 20,
        110, 22,
        "SEPTEMBER",
        0xFFFF,
        2
    );

    draw_text_box(
        95, 42,
        50, 18,
        year_text,
        0x7BEF,
        1
    );

    // Large day
    draw_text_box(
        75, 65,
        90, 50,
        date_text,
        0xFFFF,
        3
    );

    // Highlight under current day
    draw_line(
        85, 120,
        155, 120,
        0x07FF
    );

    // Weekday
    draw_text_box(
        80, 128,
        80, 20,
        "TUESDAY",
        0x07FF,
        1
    );

    // Calendar grid
    draw_text_box(35, 158, 25, 15, "M", 0x7BEF, 1);
    draw_text_box(65, 158, 25, 15, "T", 0x7BEF, 1);
    draw_text_box(95, 158, 25, 15, "W", 0x7BEF, 1);
    draw_text_box(125, 158, 25, 15, "T", 0x7BEF, 1);
    draw_text_box(155, 158, 25, 15, "F", 0x7BEF, 1);
    draw_text_box(185, 158, 25, 15, "S", 0x7BEF, 1);

    // Current date marker
    circle today = {
        .cen = {195, 185},
        .radius = 10,
        .color = 0x07FF
    };

    draw_circle(&today);

    draw_text_box(
        187, 178,
        16, 15,
        "22",
        0x0000,
        1
    );

    // Small status
    draw_text_box(
        65, 210,
        110, 18,
        "NO EVENTS",
        0x7BEF,
        1
    );
}