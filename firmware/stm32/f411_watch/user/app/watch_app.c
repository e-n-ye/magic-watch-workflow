#include "watch_app.h"

#include <stdio.h>

#include "board/display/watch_lcd.h"
#include "board/input/watch_input_hw.h"
#include "board/power/watch_power.h"
#include "board/storage/watch_ota_board.h"
#include "board/time/watch_rtc_board.h"
#include "board/usb/watch_usb_cdc.h"
#include "config/user_config.h"
#include "main.h"
#include "watch_core.h"
#include "watch_diagnostic.h"
#include "watch_runtime.h"

static watch_core_t s_core;
static bool s_app_ready;
static bool s_status_reported;
static uint32_t s_touch_reported_sequence;
static bool s_trial_active;
static uint32_t s_trial_started_ms;
static uint32_t s_trial_last_check_ms;

static const char *watch_app_event_name(watch_event_type_t type)
{
    switch (type) {
    case WATCH_EVENT_WAKE:
        return "wake";
    case WATCH_EVENT_BACK:
        return "back";
    case WATCH_EVENT_SELECT:
        return "select";
    case WATCH_EVENT_UP:
        return "up";
    case WATCH_EVENT_DOWN:
        return "down";
    case WATCH_EVENT_TIME_UPDATED:
        return "time";
    case WATCH_EVENT_SENSOR_STATUS_UPDATED:
        return "sensor";
    case WATCH_EVENT_NONE:
    case WATCH_EVENT_COUNT:
        return "none";
    }

    return "invalid";
}

static void watch_app_report_status(void)
{
    char response[192];
    watch_input_hw_status_t status;
    int length;

    watch_input_hw_read_status(&status);
    length =
        snprintf(response, sizeof(response),
                 "input hw encoder=%u touch=%u chip=0x%02x count=%u "
                 "dir=%s exti=%lu,%lu,%lu i2c_err=%lu drop=%lu\r\n",
                 status.encoder_ready ? 1U : 0U, status.touch_ready ? 1U : 0U, status.touch_chip_id,
                 status.encoder_count, WATCH_INPUT_HW_ENCODER_REVERSE ? "reverse" : "normal",
                 (unsigned long)status.exti_count[0], (unsigned long)status.exti_count[1],
                 (unsigned long)status.exti_count[2], (unsigned long)status.touch_errors,
                 (unsigned long)status.event_dropped);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        (void)watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void watch_app_report_touch_if_new(void)
{
    char response[160];
    watch_input_hw_status_t status;
    int length;

    watch_input_hw_read_status(&status);
    if (status.touch_sequence == s_touch_reported_sequence) {
        return;
    }

    s_touch_reported_sequence = status.touch_sequence;
    length = snprintf(response, sizeof(response),
                      "input touch gesture=0x%02x finger=%u x=%u y=%u map=%s queued=%u seq=%lu\r\n",
                      status.touch_gesture, status.touch_finger_count, status.touch_x,
                      status.touch_y, watch_app_event_name(status.touch_event),
                      status.touch_event_queued ? 1U : 0U, (unsigned long)status.touch_sequence);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        (void)watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void watch_app_report_event(const watch_event_t *event, bool dispatched,
                                   watch_command_type_t command_type,
                                   const watch_snapshot_t *snapshot)
{
    char response[160];
    int length = snprintf(
        response, sizeof(response),
        "input event=%s accepted=%u cmd=%u page=%u depth=%u index=%u rev=%lu\r\n",
        watch_app_event_name(event->type), dispatched ? 1U : 0U, (unsigned int)command_type,
        (unsigned int)snapshot->page, (unsigned int)snapshot->page_depth,
        (unsigned int)snapshot->launcher_index, (unsigned long)snapshot->revision);

    if ((length > 0) && ((size_t)length < sizeof(response))) {
        (void)watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static watch_command_type_t watch_app_take_commands(void)
{
    watch_command_t command;
    watch_command_type_t command_type = WATCH_COMMAND_NONE;

    while (watch_core_take_command(&s_core, &command)) {
        command_type = command.type;
    }

    return command_type;
}

static bool watch_app_dispatch_event(const watch_event_t *event, bool report_event)
{
    watch_snapshot_t snapshot = { 0 };
    watch_command_type_t command_type;
    bool dispatched = watch_core_dispatch_event(&s_core, event);

    command_type = watch_app_take_commands();
    if (report_event && watch_core_read_snapshot(&s_core, &snapshot)) {
        watch_app_report_event(event, dispatched, command_type, &snapshot);
    }

    return dispatched;
}

static void watch_app_process_time(uint32_t now_ms)
{
    watch_time_value_t time;
    watch_event_t event;

    if (!watch_rtc_board_process(now_ms, &time)) {
        return;
    }

    event = (watch_event_t) {
        .type = WATCH_EVENT_TIME_UPDATED,
        .time = time,
    };
    (void)watch_app_dispatch_event(&event, false);
}

static bool watch_app_service_healthy(watch_runtime_service_t service, uint32_t now_ms)
{
    watch_runtime_health_t health;

    return watch_runtime_read_health(service, now_ms, &health)
        && health.state == WATCH_RUNTIME_HEALTH_HEALTHY;
}

static void watch_app_process_trial(uint32_t now_ms)
{
    watch_input_hw_status_t input_status;
    watch_power_board_status_t power_status;
    watch_ota_board_status_t ota_status;
    watch_ota_trial_health_t health;

    if (!s_trial_active
        || watch_runtime_elapsed_ms(now_ms, s_trial_started_ms) < WATCH_OTA_TRIAL_CONFIRMATION_MS
        || watch_runtime_elapsed_ms(now_ms, s_trial_last_check_ms) < 500U
        || !watch_ota_board_read_status(&ota_status)) {
        return;
    }
    s_trial_last_check_ms = now_ms;
    if (!ota_status.record_valid || ota_status.record.state != WATCH_OTA_METADATA_TRIAL) {
        s_trial_active = false;
        return;
    }

    watch_input_hw_read_status(&input_status);
    health.input_healthy = input_status.encoder_ready && input_status.touch_ready;
    health.ui_healthy = watch_app_service_healthy(WATCH_RUNTIME_SERVICE_UI, now_ms);
    health.supervisor_healthy = watch_app_service_healthy(WATCH_RUNTIME_SERVICE_APP, now_ms)
        && watch_app_service_healthy(WATCH_RUNTIME_SERVICE_USB, now_ms);
    health.metadata_healthy = ota_status.flash_result == WATCH_W25Q128_RESULT_OK
        && ota_status.metadata_result == WATCH_OTA_METADATA_RESULT_OK;
    health.watchdog_healthy = watch_power_board_read_status(&power_status)
        && power_status.watchdog_enabled && power_status.watchdog_refresh_count > 0U
        && power_status.watchdog_refresh_failure_count == 0U;
    if (watch_ota_trial_health_ready(&health, watch_runtime_elapsed_ms(now_ms, s_trial_started_ms))
            == WATCH_OTA_TRIAL_RESULT_OK
        && watch_ota_board_confirm_trial() == WATCH_OTA_METADATA_RESULT_OK) {
        s_trial_active = false;
    }
}

void watch_app_init(void)
{
    watch_diagnostic_capsule_t capsule;
    watch_time_value_t time;
    uint32_t now_ms = HAL_GetTick();

    s_app_ready = false;
    s_trial_active = false;
    s_trial_started_ms = now_ms;
    s_trial_last_check_ms = now_ms;
    (void)watch_runtime_init(now_ms);

    watch_lcd_init();
    watch_lcd_backlight_on();

    if (watch_diagnostic_get(&capsule)) {
        (void)watch_ota_board_mark_trial_fault(WATCH_OTA_TRIAL_ERROR_DIAGNOSTIC);
        watch_runtime_fail();
        watch_lcd_show_diagnostic_pattern(capsule.reason);
        watch_diagnostic_clear();
        return;
    }

    if (!watch_core_init(&s_core) || !watch_runtime_advance_init(WATCH_RUNTIME_INIT_CORE)
        || !watch_input_hw_init() || !watch_runtime_advance_init(WATCH_RUNTIME_INIT_INPUT)
        || !watch_runtime_advance_init(WATCH_RUNTIME_INIT_RUNNING)
        || !watch_runtime_start_service(WATCH_RUNTIME_SERVICE_APP, now_ms)) {
        watch_runtime_fail();
        return;
    }

    if (watch_rtc_board_init(now_ms, &time)) {
        watch_event_t event = {
            .type = WATCH_EVENT_TIME_UPDATED,
            .time = time,
        };

        (void)watch_app_dispatch_event(&event, false);
    }

    s_status_reported = false;
    s_touch_reported_sequence = 0U;
    {
        watch_ota_board_status_t ota_status;

        if (watch_ota_board_read_status(&ota_status) && ota_status.record_valid
            && ota_status.record.state == WATCH_OTA_METADATA_TRIAL) {
            s_trial_active = true;
            s_trial_started_ms = now_ms;
            s_trial_last_check_ms = now_ms - 500U;
        }
    }
    s_app_ready = true;
    if (!watch_power_board_init(now_ms)) {
        s_app_ready = false;
        watch_runtime_fail();
    }
}

bool watch_app_is_ready(void)
{
    return s_app_ready && watch_runtime_is_ready();
}

void watch_app_process(void)
{
    watch_event_t event;
    uint32_t now_ms;

    if (!s_app_ready) {
        return;
    }

    now_ms = HAL_GetTick();
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_APP, now_ms);
    watch_app_process_time(now_ms);
    watch_input_hw_process(now_ms);
    watch_app_process_trial(now_ms);
    watch_app_report_touch_if_new();
    if (!s_status_reported) {
        watch_app_report_status();
        s_status_reported = true;
    }

    while (watch_input_hw_take_event(&event)) {
        if (event.type == WATCH_EVENT_WAKE) {
            watch_power_board_notify_wake(WATCH_POWER_WAKE_KEY);
        }
        (void)watch_app_dispatch_event(&event, true);
    }
}

bool watch_app_read_snapshot(watch_snapshot_t *snapshot)
{
    if (!s_app_ready) {
        return false;
    }

    return watch_core_read_snapshot(&s_core, snapshot);
}

bool watch_app_dispatch_sensor_snapshot(const watch_sensor_aggregate_snapshot_t *sensor_snapshot)
{
    watch_event_t event;

    if (!s_app_ready || !watch_sensor_aggregate_snapshot_is_valid(sensor_snapshot)) {
        return false;
    }

    event = (watch_event_t) {
        .type = WATCH_EVENT_SENSOR_STATUS_UPDATED,
        .sensor_snapshot = *sensor_snapshot,
    };
    return watch_app_dispatch_event(&event, false);
}
