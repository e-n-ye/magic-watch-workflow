#include "watch_time.h"

#include <assert.h>
#include <string.h>

static watch_time_value_t watch_test_time(void)
{
    return (watch_time_value_t) {
        .year = 2028U,
        .month = 2U,
        .day = 29U,
        .weekday = WATCH_TIME_WEEKDAY_TUESDAY,
        .hour = 23U,
        .minute = 58U,
        .second = 7U,
    };
}

static void test_calendar_validation(void)
{
    watch_time_value_t time = watch_test_time();

    assert(watch_time_is_valid(&time));
    time.year = 2027U;
    assert(!watch_time_is_valid(&time));
    time = watch_test_time();
    time.month = 13U;
    assert(!watch_time_is_valid(&time));
    time = watch_test_time();
    time.day = 30U;
    assert(!watch_time_is_valid(&time));
    time = watch_test_time();
    time.weekday = (watch_time_weekday_t)0;
    assert(!watch_time_is_valid(&time));
    time = watch_test_time();
    time.hour = 24U;
    assert(!watch_time_is_valid(&time));
}

static void test_comparison_and_formatting(void)
{
    char hms[WATCH_TIME_HMS_TEXT_SIZE];
    char local[WATCH_TIME_LOCAL_TEXT_SIZE];
    watch_time_value_t time = watch_test_time();
    watch_time_value_t same = time;

    assert(watch_time_equal(&time, &same));
    same.second++;
    assert(!watch_time_equal(&time, &same));
    assert(!watch_time_equal(&time, NULL));
    assert(watch_time_format_hms(&time, hms, sizeof(hms)));
    assert(strcmp(hms, "23:58:07") == 0);
    assert(watch_time_format_local(&time, local, sizeof(local)));
    assert(strcmp(local, "2028-02-29 23:58:07") == 0);
    assert(!watch_time_format_hms(&time, hms, sizeof(hms) - 1U));
    assert(hms[0] == '\0');
}

int main(void)
{
    test_calendar_validation();
    test_comparison_and_formatting();
    return 0;
}
