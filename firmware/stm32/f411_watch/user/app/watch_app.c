#include "watch_app.h"

#include <stdio.h>

#include "board/display/watch_lcd.h"
#include "board/input/watch_input_hw.h"
#include "board/usb/watch_usb_cdc.h"
#include "config/user_config.h"
#include "main.h"
#include "watch_core.h"
#include "watch_diagnostic.h"

static watch_core_t s_core;
static bool s_app_ready;
static bool s_status_reported;
static uint32_t s_touch_reported_sequence;

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
    case WATCH_EVENT_NONE:
    case WATCH_EVENT_COUNT:
        return "none";
    }

    return "invalid";
}

static uint16_t watch_app_event_color(watch_event_type_t type)
{
    switch (type) {
    case WATCH_EVENT_WAKE:
        return WATCH_LCD_YELLOW;
    case WATCH_EVENT_BACK:
        return WATCH_LCD_RED;
    case WATCH_EVENT_SELECT:
        return WATCH_LCD_GREEN;
    case WATCH_EVENT_UP:
        return WATCH_LCD_BLUE;
    case WATCH_EVENT_DOWN:
        return WATCH_LCD_CYAN;
    case WATCH_EVENT_NONE:
    case WATCH_EVENT_COUNT:
        return WATCH_LCD_MAGENTA;
    }

    return WATCH_LCD_MAGENTA;
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

    watch_lcd_fill_rect(0U, 0U, WATCH_LCD_WIDTH, 12U,
                        dispatched ? watch_app_event_color(event->type) : WATCH_LCD_MAGENTA);
}

void watch_app_init(void)
{
    watch_diagnostic_capsule_t capsule;

    watch_lcd_init();
    watch_lcd_backlight_on();

    if (watch_diagnostic_get(&capsule)) {
        watch_lcd_show_diagnostic_pattern(capsule.reason);
        watch_diagnostic_clear();
        return;
    }

    (void)watch_core_init(&s_core);
    (void)watch_input_hw_init();
    s_status_reported = false;
    s_touch_reported_sequence = 0U;
    s_app_ready = true;
    watch_lcd_show_bringup_pattern();
}

void watch_app_process(void)
{
    watch_event_t event;

    if (!s_app_ready) {
        return;
    }

    watch_input_hw_process(HAL_GetTick());
    watch_app_report_touch_if_new();
    if (!s_status_reported) {
        watch_app_report_status();
        s_status_reported = true;
    }

    while (watch_input_hw_take_event(&event)) {
        watch_snapshot_t snapshot = { 0 };
        watch_command_t command;
        watch_command_type_t command_type = WATCH_COMMAND_NONE;
        bool dispatched = watch_core_dispatch_event(&s_core, &event);

        while (watch_core_take_command(&s_core, &command)) {
            command_type = command.type;
        }
        (void)watch_core_read_snapshot(&s_core, &snapshot);
        watch_app_report_event(&event, dispatched, command_type, &snapshot);
    }
}
