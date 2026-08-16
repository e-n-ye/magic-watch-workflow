#ifndef WATCH_TIME_H
#define WATCH_TIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_TIME_HMS_TEXT_SIZE 9U
#define WATCH_TIME_LOCAL_TEXT_SIZE 20U

typedef enum {
    WATCH_TIME_WEEKDAY_MONDAY = 1,
    WATCH_TIME_WEEKDAY_TUESDAY,
    WATCH_TIME_WEEKDAY_WEDNESDAY,
    WATCH_TIME_WEEKDAY_THURSDAY,
    WATCH_TIME_WEEKDAY_FRIDAY,
    WATCH_TIME_WEEKDAY_SATURDAY,
    WATCH_TIME_WEEKDAY_SUNDAY
} watch_time_weekday_t;

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    watch_time_weekday_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} watch_time_value_t;

bool watch_time_is_valid(const watch_time_value_t *value);
bool watch_time_equal(const watch_time_value_t *left, const watch_time_value_t *right);
bool watch_time_format_hms(const watch_time_value_t *value, char *text, size_t text_size);
bool watch_time_format_local(const watch_time_value_t *value, char *text, size_t text_size);

#endif /* WATCH_TIME_H */
