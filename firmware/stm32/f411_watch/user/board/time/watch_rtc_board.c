#include "watch_rtc_board.h"

#include "main.h"
#include "rtc.h"

#define WATCH_RTC_BOARD_BACKUP_MAGIC 0x57415443UL
#define WATCH_RTC_BOARD_UPDATE_PERIOD_MS 1000U

static RTC_TimeTypeDef s_retained_time;
static RTC_DateTypeDef s_retained_date;
static watch_time_value_t s_latest_time;
static bool s_retained_time_valid;
static bool s_initialized;
static uint32_t s_last_update_ms;

static bool watch_rtc_board_read_raw(RTC_TimeTypeDef *time, RTC_DateTypeDef *date)
{
    return HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN) == HAL_OK
        && HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN) == HAL_OK;
}

static bool watch_rtc_board_read_time(watch_time_value_t *time)
{
    RTC_TimeTypeDef raw_time;
    RTC_DateTypeDef raw_date;
    watch_time_value_t value;

    if (time == NULL || !watch_rtc_board_read_raw(&raw_time, &raw_date)) {
        return false;
    }

    value = (watch_time_value_t) {
        .year = (uint16_t)(2000U + raw_date.Year),
        .month = raw_date.Month,
        .day = raw_date.Date,
        .weekday = (watch_time_weekday_t)raw_date.WeekDay,
        .hour = raw_time.Hours,
        .minute = raw_time.Minutes,
        .second = raw_time.Seconds,
    };
    if (!watch_time_is_valid(&value)) {
        return false;
    }

    *time = value;
    return true;
}

void watch_rtc_board_capture_boot(void)
{
    watch_time_value_t retained_value;

    s_retained_time_valid = false;
    HAL_PWR_EnableBkUpAccess();
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != WATCH_RTC_BOARD_BACKUP_MAGIC
        || !watch_rtc_board_read_raw(&s_retained_time, &s_retained_date)) {
        return;
    }

    retained_value = (watch_time_value_t) {
        .year = (uint16_t)(2000U + s_retained_date.Year),
        .month = s_retained_date.Month,
        .day = s_retained_date.Date,
        .weekday = (watch_time_weekday_t)s_retained_date.WeekDay,
        .hour = s_retained_time.Hours,
        .minute = s_retained_time.Minutes,
        .second = s_retained_time.Seconds,
    };
    s_retained_time_valid = watch_time_is_valid(&retained_value);
}

void watch_rtc_board_restore_boot(void)
{
    HAL_PWR_EnableBkUpAccess();
    if (s_retained_time_valid) {
        (void)HAL_RTC_SetTime(&hrtc, &s_retained_time, RTC_FORMAT_BIN);
        (void)HAL_RTC_SetDate(&hrtc, &s_retained_date, RTC_FORMAT_BIN);
    }

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, WATCH_RTC_BOARD_BACKUP_MAGIC);
    s_retained_time_valid = false;
}

bool watch_rtc_board_init(uint32_t now_ms, watch_time_value_t *time)
{
    if (time == NULL || !watch_rtc_board_read_time(time)) {
        return false;
    }

    s_latest_time = *time;
    s_last_update_ms = now_ms;
    s_initialized = true;
    return true;
}

bool watch_rtc_board_process(uint32_t now_ms, watch_time_value_t *time)
{
    watch_time_value_t next_time;

    if (!s_initialized || time == NULL
        || (now_ms - s_last_update_ms) < WATCH_RTC_BOARD_UPDATE_PERIOD_MS) {
        return false;
    }

    s_last_update_ms = now_ms;
    if (!watch_rtc_board_read_time(&next_time)) {
        return false;
    }

    if (watch_time_equal(&s_latest_time, &next_time)) {
        return false;
    }

    s_latest_time = next_time;
    *time = next_time;
    return true;
}
