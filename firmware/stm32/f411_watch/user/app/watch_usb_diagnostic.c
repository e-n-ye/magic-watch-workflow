#include "watch_usb_diagnostic.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board/usb/watch_usb_cdc.h"
#include "board/sensors/watch_aht20_board.h"
#include "board/sensors/watch_cw2015_board.h"
#include "board/sensors/watch_sensor_aggregate_board.h"
#include "board/sensors/watch_max30102_board.h"
#include "board/sensors/watch_lis2mdl_board.h"
#include "board/sensors/watch_lsm6ds3_board.h"
#include "board/power/watch_power.h"
#include "board/storage/watch_eeprom_probe_board.h"
#include "board/storage/watch_littlefs_board.h"
#include "board/storage/watch_w25q128_board.h"
#include "main.h"
#include "watch_app.h"
#include "watch_diagnostic.h"
#include "watch_runtime.h"

#define WATCH_USB_DIAGNOSTIC_COMMAND_SIZE 96U
#define WATCH_USB_DIAGNOSTIC_READ_SIZE 32U

typedef enum {
    WATCH_USB_COMMAND_HELP = 1,
    WATCH_USB_COMMAND_PING,
    WATCH_USB_COMMAND_INFO,
    WATCH_USB_COMMAND_DIAG,
    WATCH_USB_COMMAND_STATS,
    WATCH_USB_COMMAND_HEALTH,
    WATCH_USB_COMMAND_SENSOR,
    WATCH_USB_COMMAND_EEPROM,
    WATCH_USB_COMMAND_W25,
    WATCH_USB_COMMAND_FS,
    WATCH_USB_COMMAND_FS_TEST,
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
    char response[160];
    char time[WATCH_TIME_LOCAL_TEXT_SIZE];
    watch_snapshot_t snapshot;
    const char *time_text = "unavailable";
    int length;

    if (!watch_app_read_snapshot(&snapshot)) {
        send_text("watch=f411 usb=cdc protocol=1 display=240x280 page=unavailable time=unavailable\r\n");
        return;
    }

    if (snapshot.time_valid && watch_time_format_local(&snapshot.time, time, sizeof(time))) {
        time_text = time;
    }

    length = snprintf(response, sizeof(response),
                      "watch=f411 usb=cdc protocol=1 display=240x280 page=%u depth=%u popup=%u "
                      "sensors=0x%02x degraded=%u time=%s\r\n",
                      (unsigned int)snapshot.page, (unsigned int)snapshot.page_depth,
                      snapshot.popup_visible ? 1U : 0U,
                      (unsigned int)snapshot.sensor_snapshot.available_mask,
                      snapshot.sensor_snapshot.degraded ? 1U : 0U, time_text);
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
                      (unsigned int)watch_runtime_ui_event_count());
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_sensor(void)
{
    char response[256];
    watch_lsm6ds3_service_status_t status;
    watch_lsm6ds3_sample_t sample = { 0 };
    watch_lis2mdl_service_status_t lis2mdl_status;
    watch_lis2mdl_sample_t lis2mdl_sample = { 0 };
    watch_aht20_service_status_t aht20_status;
    watch_aht20_sample_t aht20_sample = { 0 };
    watch_cw2015_service_status_t cw2015_status;
    watch_cw2015_sample_t cw2015_sample = { 0 };
    watch_max30102_service_status_t max30102_status;
    watch_max30102_sample_t max30102_sample = { 0 };
    bool sample_valid;
    bool aht20_sample_valid;
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

    if (!watch_aht20_board_read_status(&aht20_status)) {
        send_text("sensor aht20=unavailable\r\n");
        return;
    }

    aht20_sample_valid = watch_aht20_board_read_latest(&aht20_sample);
    length = snprintf(response, sizeof(response),
                      "sensor aht20=%u cal=%u sample=%u count=%lu errors=%lu crc=%lu "
                      "timeout=%lu drop=%lu temp_x100=%d humidity_x100=%u state=%u status=0x%02x\r\n",
                      aht20_status.ready ? 1U : 0U, aht20_status.calibrated ? 1U : 0U,
                      aht20_sample_valid ? 1U : 0U, (unsigned long)aht20_status.sample_count,
                      (unsigned long)aht20_status.read_error_count,
                      (unsigned long)aht20_status.crc_error_count,
                      (unsigned long)aht20_status.timeout_count,
                      (unsigned long)aht20_status.event_drop_count,
                      (int)aht20_sample.temperature_centi_c,
                      (unsigned int)aht20_sample.humidity_centi_percent,
                      (unsigned int)aht20_status.state, aht20_status.status_byte);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }

    if (!watch_max30102_board_read_status(&max30102_status)) {
        send_text("sensor max30102=unavailable\r\n");
    } else {
        bool max30102_sample_valid = watch_max30102_board_read_latest(&max30102_sample);

        length = snprintf(
            response, sizeof(response),
            "sensor max30102=%u part=0x%02x rev=0x%02x sample=%u count=%lu errors=%lu "
            "id_errors=%lu reset_timeouts=%lu no_data=%lu fifo_overflow=%lu drop=%lu "
            "red=%lu ir=%lu finger=%u state=%u mode=0x%02x\r\n",
            max30102_status.ready ? 1U : 0U, max30102_status.part_id,
            max30102_status.revision_id, max30102_sample_valid ? 1U : 0U,
            (unsigned long)max30102_status.sample_count,
            (unsigned long)max30102_status.read_error_count,
            (unsigned long)max30102_status.id_error_count,
            (unsigned long)max30102_status.reset_timeout_count,
            (unsigned long)max30102_status.no_data_count,
            (unsigned long)max30102_status.fifo_overflow_count,
            (unsigned long)max30102_status.event_drop_count,
            (unsigned long)max30102_sample.red_raw, (unsigned long)max30102_sample.ir_raw,
            max30102_sample.finger_on ? 1U : 0U, (unsigned int)max30102_status.state,
            max30102_status.mode_config);
        if ((length > 0) && ((size_t)length < sizeof(response))) {
            watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
        }
    }

    if (!watch_cw2015_board_read_status(&cw2015_status)) {
        send_text("sensor cw2015=unavailable\r\n");
    } else {
        bool cw2015_sample_valid = watch_cw2015_board_read_latest(&cw2015_sample);

        length = snprintf(
            response, sizeof(response),
            "sensor cw2015=%u version=0x%02x sample=%u count=%lu errors=%lu "
            "invalid_soc=%lu drop=%lu voltage_mv=%u soc=%u fraction=%u state=%u\r\n",
            cw2015_status.ready ? 1U : 0U, cw2015_status.version,
            cw2015_sample_valid ? 1U : 0U, (unsigned long)cw2015_status.sample_count,
            (unsigned long)cw2015_status.read_error_count,
            (unsigned long)cw2015_status.invalid_soc_count,
            (unsigned long)cw2015_status.event_drop_count,
            (unsigned int)cw2015_sample.voltage_mv, (unsigned int)cw2015_sample.soc_percent,
            (unsigned int)cw2015_sample.soc_fraction, (unsigned int)cw2015_status.state);
        if ((length > 0) && ((size_t)length < sizeof(response))) {
            watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
        }
    }

    if (!watch_lis2mdl_board_read_status(&lis2mdl_status)) {
        send_text("sensor lis2mdl=unavailable\r\n");
    } else {
        bool lis2mdl_sample_valid = watch_lis2mdl_board_read_latest(&lis2mdl_sample);

        length = snprintf(response, sizeof(response),
                          "sensor lis2mdl=%u id=0x%02x sample=%u count=%lu errors=%lu nack=%lu "
                          "drop=%lu mag=%d,%d,%d state=%u\r\n",
                          lis2mdl_status.ready ? 1U : 0U, lis2mdl_status.who_am_i,
                          lis2mdl_sample_valid ? 1U : 0U,
                          (unsigned long)lis2mdl_status.sample_count,
                          (unsigned long)lis2mdl_status.read_error_count,
                          (unsigned long)lis2mdl_status.nack_count,
                          (unsigned long)lis2mdl_status.event_drop_count,
                          (int)lis2mdl_sample.magnetic_x, (int)lis2mdl_sample.magnetic_y,
                          (int)lis2mdl_sample.magnetic_z, (unsigned int)lis2mdl_status.state);
        if ((length > 0) && ((size_t)length < sizeof(response))) {
            watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
        }
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

static void send_eeprom(void)
{
    char response[192];
    watch_eeprom_probe_status_t status;
    int length;

    if (!watch_eeprom_probe_board_read_status(&status)) {
        send_text("eeprom=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "eeprom candidate=BL24C02F-RRRC complete=%u range=0x%02x-0x%02x "
                      "probed=%u response_mask=0x%02x scans=%lu\r\n",
                      status.complete ? 1U : 0U, (unsigned int)status.first_address,
                      (unsigned int)status.last_address, (unsigned int)status.probed_count,
                      (unsigned int)status.response_mask, (unsigned long)status.scan_count);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_w25(void)
{
    char response[160];
    uint32_t jedec_id = 0U;
    watch_w25q128_result_t id_result = watch_w25q128_board_read_id(&jedec_id);
    watch_w25q128_result_t ready_result =
        watch_w25q128_board_wait_ready(WATCH_W25Q128_BOARD_DEFAULT_TIMEOUT_MS);
    int length = snprintf(response, sizeof(response),
                          "w25 id=0x%06lx id_result=%s ready=%s\r\n",
                          (unsigned long)jedec_id, watch_w25q128_result_name(id_result),
                          watch_w25q128_result_name(ready_result));

    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_fs(void)
{
    char response[224];
    watch_littlefs_board_status_t status;
    watch_littlefs_result_t mount_result = watch_littlefs_board_mount();
    int length;

    if (!watch_littlefs_board_read_status(&status)) {
        send_text("fs=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "fs mounted=%u result=%s mount=%s partition=0x%06lx-0x%06lx "
                      "chunks=%lu,%lu,%lu\r\n",
                      status.mounted ? 1U : 0U, watch_littlefs_result_name(status.last_result),
                      watch_littlefs_result_name(mount_result), (unsigned long)WATCH_W25_LITTLEFS_OFFSET,
                      (unsigned long)WATCH_W25_LITTLEFS_END, (unsigned long)status.image_chunks,
                      (unsigned long)status.font_chunks, (unsigned long)status.text_chunks);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void send_fs_test(void)
{
    char response[224];
    watch_littlefs_board_status_t status;
    watch_littlefs_result_t result = watch_littlefs_board_run_resource_test();
    int length;

    if (!watch_littlefs_board_read_status(&status)) {
        send_text("fs-test=unavailable\r\n");
        return;
    }

    length = snprintf(response, sizeof(response),
                      "fs-test result=%s mounted=%u image=%lu font=%lu text=%lu "
                      "partition=0x%06lx-0x%06lx\r\n",
                      watch_littlefs_result_name(result), status.mounted ? 1U : 0U,
                      (unsigned long)status.image_chunks, (unsigned long)status.font_chunks,
                      (unsigned long)status.text_chunks, (unsigned long)WATCH_W25_LITTLEFS_OFFSET,
                      (unsigned long)WATCH_W25_LITTLEFS_END);
    if ((length > 0) && ((size_t)length < sizeof(response))) {
        watch_usb_cdc_write((const uint8_t *)response, (size_t)length);
    }
}

static void handle_command(watch_usb_command_t command)
{
    switch (command) {
    case WATCH_USB_COMMAND_HELP:
        send_text("commands: help ping info diag stats health sensor eeprom w25 fs fs-test power "
                  "display-off sleep shutdown\r\n");
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
    case WATCH_USB_COMMAND_EEPROM:
        send_eeprom();
        break;
    case WATCH_USB_COMMAND_W25:
        send_w25();
        break;
    case WATCH_USB_COMMAND_FS:
        send_fs();
        break;
    case WATCH_USB_COMMAND_FS_TEST:
        send_fs_test();
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
            watch_usb_cdc_reinitialize();
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
    } else if (strcmp(s_command, "eeprom") == 0) {
        return WATCH_USB_COMMAND_EEPROM;
    } else if (strcmp(s_command, "w25") == 0) {
        return WATCH_USB_COMMAND_W25;
    } else if (strcmp(s_command, "fs") == 0) {
        return WATCH_USB_COMMAND_FS;
    } else if (strcmp(s_command, "fs-test") == 0) {
        return WATCH_USB_COMMAND_FS_TEST;
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
        if (s_command_overflow) {
            send_text("error=line-too-long\r\n");
        } else if (s_command_length > 0U) {
            handle_command(parse_command());
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

void watch_usb_diagnostic_process(void)
{
    uint8_t input[WATCH_USB_DIAGNOSTIC_READ_SIZE];
    size_t length = watch_usb_cdc_read(input, sizeof(input));
    uint32_t now_ms = HAL_GetTick();

    watch_lsm6ds3_board_process(now_ms);
    watch_lis2mdl_board_process(now_ms);
    watch_aht20_board_process(now_ms);
    watch_cw2015_board_process(now_ms);
    watch_max30102_board_process(now_ms);
    watch_sensor_aggregate_board_process(now_ms);
    watch_eeprom_probe_board_process(now_ms);
    (void)watch_runtime_start_service(WATCH_RUNTIME_SERVICE_USB, now_ms);
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_USB, now_ms);

    for (size_t index = 0U; index < length; ++index) {
        consume_byte(input[index]);
    }

    watch_power_board_process(HAL_GetTick());
    watch_usb_cdc_process();
}
