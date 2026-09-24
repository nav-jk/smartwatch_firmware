#ifndef CALENDAR_H
#define CALENDAR_H

typedef struct {
    int day;
    int month;
    int year;
} calendar_date_t;

calendar_date_t calendar_fetch_date(void);
void menu_calendar_init(void);

#endif