#include "watch_time.h"

static bool watch_time_is_leap_year(uint16_t year)
{
    return ((year % 4U) == 0U && (year % 100U) != 0U) || (year % 400U) == 0U;
}

static uint8_t watch_time_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U,
    };

    if (month == 2U && watch_time_is_leap_year(year)) {
        return 29U;
    }

    return days[month - 1U];
}

static void watch_time_write_two_digits(char *text, uint8_t value)
{
    text[0] = (char)('0' + (value / 10U));
    text[1] = (char)('0' + (value % 10U));
}

static void watch_time_write_year(char *text, uint16_t year)
{
    text[0] = (char)('0' + (year / 1000U));
    text[1] = (char)('0' + ((year / 100U) % 10U));
    text[2] = (char)('0' + ((year / 10U) % 10U));
    text[3] = (char)('0' + (year % 10U));
}

bool watch_time_is_valid(const watch_time_value_t *value)
{
    if (value == NULL || value->year < 2000U || value->year > 2099U || value->month == 0U
        || value->month > 12U || value->weekday < WATCH_TIME_WEEKDAY_MONDAY
        || value->weekday > WATCH_TIME_WEEKDAY_SUNDAY || value->hour > 23U || value->minute > 59U
        || value->second > 59U) {
        return false;
    }

    return value->day > 0U && value->day <= watch_time_days_in_month(value->year, value->month);
}

bool watch_time_equal(const watch_time_value_t *left, const watch_time_value_t *right)
{
    return left != NULL && right != NULL && left->year == right->year && left->month == right->month
        && left->day == right->day && left->weekday == right->weekday && left->hour == right->hour
        && left->minute == right->minute && left->second == right->second;
}

bool watch_time_format_hms(const watch_time_value_t *value, char *text, size_t text_size)
{
    if (text != NULL && text_size > 0U) {
        text[0] = '\0';
    }

    if (!watch_time_is_valid(value) || text == NULL || text_size < WATCH_TIME_HMS_TEXT_SIZE) {
        return false;
    }

    watch_time_write_two_digits(&text[0], value->hour);
    text[2] = ':';
    watch_time_write_two_digits(&text[3], value->minute);
    text[5] = ':';
    watch_time_write_two_digits(&text[6], value->second);
    text[8] = '\0';
    return true;
}

bool watch_time_format_local(const watch_time_value_t *value, char *text, size_t text_size)
{
    if (text != NULL && text_size > 0U) {
        text[0] = '\0';
    }

    if (!watch_time_is_valid(value) || text == NULL || text_size < WATCH_TIME_LOCAL_TEXT_SIZE) {
        return false;
    }

    watch_time_write_year(&text[0], value->year);
    text[4] = '-';
    watch_time_write_two_digits(&text[5], value->month);
    text[7] = '-';
    watch_time_write_two_digits(&text[8], value->day);
    text[10] = ' ';
    watch_time_write_two_digits(&text[11], value->hour);
    text[13] = ':';
    watch_time_write_two_digits(&text[14], value->minute);
    text[16] = ':';
    watch_time_write_two_digits(&text[17], value->second);
    text[19] = '\0';
    return true;
}
