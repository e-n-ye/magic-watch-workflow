#include "watch_usb_diagnostic.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board/usb/watch_usb_cdc.h"
#include "board/sensors/watch_lsm6ds3_board.h"
#include "board/power/watch_power.h"
#include "main.h"
#include "watch_app.h"
#include "watch_diagnostic.h"
#include "watch_runtime.h"
#include "ui/watch_ui.h"

#define WATCH_USB_DIAGNOSTIC_COMMAND_SIZE 96U
#define WATCH_USB_DIAGNOSTIC_READ_SIZE 32U
#define WATCH_USB_DIAGNOSTIC_EVENT_COMMAND 1U

typedef enum {
    WATCH_USB_COMMAND_HELP = 1,
    WATCH_USB_COMMAND_PING,
    WATCH_USB_COMMAND_INFO,
    WATCH_USB_COMMAND_DIAG,
    WATCH_USB_COMMAND_STATS,
    WATCH_USB_COMMAND_HEALTH,
    WATCH_USB_COMMAND_SENSOR,
    WATCH_USB_COMMAND_POWER,
    WATCH_USB_COMMAND_DISPLAY_OFF,
    WATCH_USB_COMMAND_SLEEP,
    WATCH_USB_COMMAND_SHUTDOWN,
    WATCH_USB_COMMAND_UNKNOWN,
    WATCH_USB_COMMAND_COUNT
} watch_usb_command_t;

static char s_command[WATCH_USB_DIAGNOSTIC_COMMAND_SIZE];
static size_t s_command_length;
static bool s_command_overflow;

static void send_text(const char *text)
{
    watch_usb_cdc_write((const uint8_t *)text, strlen(text));
}

static void send_info(void)
{
    char response[128];
    watch_snapshot_t snapshot;
    int length;

    if (!watch_app_read_snapshot(&snapshot)) {
        send_text("watch=f411 usb=cdc protocol=1 display=240x280 page=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "watch=f411 usb=cdc protocol=1 display=240x280 page=%u depth=%u\r\n",
                      (unsigned int)snapshot.page, (unsigned int)snapshot.page_depth);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_diag(void)
{
    char response[192];
    watch_diagnostic_capsule_t capsule;
    int length;

    if (!watch_diagnostic_get(&capsule)) {
        send_text("diag=none\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "diag reason=%lu count=%lu pc=0x%08lx cfsr=0x%08lx\r\n",
                      (unsigned long)capsule.reason, (unsigned long)capsule.count,
                      (unsigned long)capsule.pc, (unsigned long)capsule.cfsr);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_stats(void)
{
    char response[128];
    int length = snprintf(response, sizeof(response),
                          "stats rx_pending=%lu rx_drop=%lu tx_drop=%lu\r\n",
                          (unsigned long)watch_usb_cdc_rx_pending(),
                          (unsigned long)watch_usb_cdc_rx_dropped(),
                          (unsigned long)watch_usb_cdc_tx_dropped());
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static const char *health_state_name(watch_runtime_health_state_t state)
{
    switch (state) {
    case WATCH_RUNTIME_HEALTH_NOT_STARTED:
        return "new";
    case WATCH_RUNTIME_HEALTH_HEALTHY:
        return "ok";
    case WATCH_RUNTIME_HEALTH_STALE:
        return "stale";
    case WATCH_RUNTIME_HEALTH_COUNT:
        return "invalid";
    }

    return "invalid";
}

static void send_health(void)
{
    char response[256];
    watch_runtime_health_t app_health;
    watch_runtime_health_t ui_health;
    watch_runtime_health_t usb_health;
    watch_runtime_health_t sensor_health;
    uint32_t now_ms = HAL_GetTick();
    int length;

    if (!watch_runtime_read_health(WATCH_RUNTIME_SERVICE_APP, now_ms, &app_health)
        || !watch_runtime_read_health(WATCH_RUNTIME_SERVICE_UI, now_ms, &ui_health)
        || !watch_runtime_read_health(WATCH_RUNTIME_SERVICE_USB, now_ms, &usb_health)
        || !watch_runtime_read_health(WATCH_RUNTIME_SERVICE_SENSOR, now_ms, &sensor_health)) {
        send_text("health=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "health stage=%u app=%s/%lu ui=%s/%lu usb=%s/%lu sensor=%s/%lu queue=%u\r\n",
                      (unsigned int)watch_runtime_init_stage(), health_state_name(app_health.state),
                      (unsigned long)app_health.heartbeat_count, health_state_name(ui_health.state),
                      (unsigned long)ui_health.heartbeat_count, health_state_name(usb_health.state),
                      (unsigned long)usb_health.heartbeat_count, health_state_name(sensor_health.state),
                      (unsigned long)sensor_health.heartbeat_count,
                      (unsigned int)watch_runtime_service_event_count());
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_sensor(void)
{
    char response[256];
    watch_lsm6ds3_service_status_t status;
    watch_lsm6ds3_sample_t sample = { 0 };
    bool sample_valid;
    int length;

    if (!watch_lsm6ds3_board_read_status(&status)) {
        send_text("sensor lsm6ds3=unavailable\r\n");
        return;
    }

    sample_valid = watch_lsm6ds3_board_read_latest(&sample);
    length = snprintf(response, sizeof(response),
                      "sensor lsm6ds3=%u id=0x%02x sample=%u count=%lu errors=%lu drop=%lu "
                      "accel=%d,%d,%d gyro=%d,%d,%d\r\n",
                      status.ready ? 1U : 0U, status.who_am_i, sample_valid ? 1U : 0U,
                      (unsigned long)status.sample_count, (unsigned long)status.read_error_count,
                      (unsigned long)status.event_drop_count, (int)sample.accel_x,
                      (int)sample.accel_y, (int)sample.accel_z, (int)sample.gyro_x,
                      (int)sample.gyro_y, (int)sample.gyro_z);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static const char *power_state_name(watch_power_state_id_t state)
{
    switch (state) {
    case WATCH_POWER_STATE_ACTIVE:
        return "active";
    case WATCH_POWER_STATE_DISPLAY_OFF:
        return "display-off";
    case WATCH_POWER_STATE_STOPPED:
        return "stopped";
    case WATCH_POWER_STATE_OFF:
        return "off";
    case WATCH_POWER_STATE_COUNT:
        return "invalid";
    }

    return "invalid";
}

static const char *power_wake_source_name(watch_power_wake_source_t source)
{
    switch (source) {
    case WATCH_POWER_WAKE_NONE:
        return "none";
    case WATCH_POWER_WAKE_KEY:
        return "key";
    case WATCH_POWER_WAKE_RTC:
        return "rtc";
    case WATCH_POWER_WAKE_RESET:
        return "reset";
    case WATCH_POWER_WAKE_COUNT:
        return "invalid";
    }

    return "invalid";
}

static void send_power(void)
{
    char response[256];
    watch_power_board_status_t status;
    int length;

    if (!watch_power_board_read_status(&status)) {
        send_text("power=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "power state=%s wake=%s transitions=%lu stops=%lu wakes=%lu "
                      "watchdog=%u refresh=%lu blocked=%lu fail=%lu\r\n",
                      power_state_name(status.power.state),
                      power_wake_source_name(status.power.wake_source),
                      (unsigned long)status.power.transition_count,
                      (unsigned long)status.stop_count, (unsigned long)status.wake_count,
                      status.watchdog_enabled ? 1U : 0U,
                      (unsigned long)status.watchdog_refresh_count,
                      (unsigned long)status.watchdog_blocked_count,
                      (unsigned long)status.watchdog_refresh_failure_count);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void handle_command(watch_usb_command_t command)
{
    switch (command) {
    case WATCH_USB_COMMAND_HELP:
        send_text("commands: help ping info diag stats health sensor power display-off sleep shutdown\r\n");
        break;
    case WATCH_USB_COMMAND_PING:
        send_text("pong\r\n");
        break;
    case WATCH_USB_COMMAND_INFO:
        send_info();
        break;
    case WATCH_USB_COMMAND_DIAG:
        send_diag();
        break;
    case WATCH_USB_COMMAND_STATS:
        send_stats();
        break;
    case WATCH_USB_COMMAND_HEALTH:
        send_health();
        break;
    case WATCH_USB_COMMAND_SENSOR:
        send_sensor();
        break;
    case WATCH_USB_COMMAND_POWER:
        send_power();
        break;
    case WATCH_USB_COMMAND_DISPLAY_OFF:
        if (!watch_power_board_request_display_off()) {
            send_text("power error=display-off\r\n");
        } else {
            send_power();
        }
        break;
    case WATCH_USB_COMMAND_SLEEP:
        if (!watch_power_board_request_stop()) {
            send_text("power error=stop\r\n");
        } else {
            send_power();
        }
        break;
    case WATCH_USB_COMMAND_SHUTDOWN:
        if (!watch_power_board_request_software_off()) {
            send_text("power error=shutdown\r\n");
        } else {
            send_power();
        }
        break;
    case WATCH_USB_COMMAND_UNKNOWN:
    case WATCH_USB_COMMAND_COUNT:
        send_text("error=unknown-command\r\n");
        break;
    }
}

static watch_usb_command_t parse_command(void)
{
    s_command[s_command_length] = '\0';

    if (strcmp(s_command, "help") == 0) {
        return WATCH_USB_COMMAND_HELP;
    } else if (strcmp(s_command, "ping") == 0) {
        return WATCH_USB_COMMAND_PING;
    } else if (strcmp(s_command, "info") == 0) {
        return WATCH_USB_COMMAND_INFO;
    } else if (strcmp(s_command, "diag") == 0) {
        return WATCH_USB_COMMAND_DIAG;
    } else if (strcmp(s_command, "stats") == 0) {
        return WATCH_USB_COMMAND_STATS;
    } else if (strcmp(s_command, "health") == 0) {
        return WATCH_USB_COMMAND_HEALTH;
    } else if (strcmp(s_command, "sensor") == 0) {
        return WATCH_USB_COMMAND_SENSOR;
    } else if (strcmp(s_command, "power") == 0) {
        return WATCH_USB_COMMAND_POWER;
    } else if (strcmp(s_command, "display-off") == 0) {
        return WATCH_USB_COMMAND_DISPLAY_OFF;
    } else if (strcmp(s_command, "sleep") == 0) {
        return WATCH_USB_COMMAND_SLEEP;
    } else if (strcmp(s_command, "shutdown") == 0) {
        return WATCH_USB_COMMAND_SHUTDOWN;
    }

    return WATCH_USB_COMMAND_UNKNOWN;
}

static void consume_byte(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n')) {
        watch_service_event_t event;

        if (s_command_overflow) {
            send_text("error=line-too-long\r\n");
        } else if (s_command_length > 0U) {
            event = (watch_service_event_t) {
                .type = WATCH_USB_DIAGNOSTIC_EVENT_COMMAND,
                .value = (uint32_t)parse_command(),
                .timestamp_ms = HAL_GetTick(),
            };
            if (!watch_runtime_post_service_event(&event)) {
                send_text("error=service-queue-full\r\n");
            }
        }

        s_command_length = 0U;
        s_command_overflow = false;
        return;
    }

    if (s_command_overflow) {
        return;
    }

    if (s_command_length < (sizeof(s_command) - 1U)) {
        s_command[s_command_length] = (char)byte;
        ++s_command_length;
    } else {
        s_command_overflow = true;
    }
}

static void process_service_events(void)
{
    watch_service_event_t event;

    while (watch_runtime_take_service_event(&event)) {
        if (event.type == WATCH_USB_DIAGNOSTIC_EVENT_COMMAND
            && event.value >= WATCH_USB_COMMAND_HELP && event.value < WATCH_USB_COMMAND_COUNT) {
            handle_command((watch_usb_command_t)event.value);
        } else if (event.type == WATCH_LSM6DS3_SERVICE_EVENT_SAMPLE) {
            /* The latest sample is exposed by the sensor command. */
        } else {
            send_text("error=unknown-service-event\r\n");
        }
    }
}

void watch_usb_diagnostic_process(void)
{
    uint8_t input[WATCH_USB_DIAGNOSTIC_READ_SIZE];
    size_t length = watch_usb_cdc_read(input, sizeof(input));
    uint32_t now_ms = HAL_GetTick();

    watch_lsm6ds3_board_process(now_ms);
    (void)watch_runtime_start_service(WATCH_RUNTIME_SERVICE_USB, now_ms);
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_USB, now_ms);

    if (watch_app_is_ready()) {
        (void)watch_ui_start();
    }

    for (size_t index = 0U; index < length; ++index) {
        consume_byte(input[index]);
    }

    process_service_events();
    watch_power_board_process(HAL_GetTick());
    watch_usb_cdc_process();
}
