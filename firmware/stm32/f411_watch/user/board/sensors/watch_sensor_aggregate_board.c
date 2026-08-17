#include "board/sensors/watch_sensor_aggregate_board.h"

#include <stdint.h>

#include "board/sensors/watch_aht20_board.h"
#include "board/sensors/watch_cw2015_board.h"
#include "board/sensors/watch_lis2mdl_board.h"
#include "board/sensors/watch_lsm6ds3_board.h"
#include "board/sensors/watch_max30102_board.h"
#include "watch_runtime.h"

static watch_sensor_aggregate_service_t s_service;
static bool s_service_initialized;
static bool s_event_pending;

static uint32_t watch_sensor_aggregate_add_count(uint32_t left, uint32_t right)
{
    if (UINT32_MAX - left < right) {
        return UINT32_MAX;
    }

    return left + right;
}

static watch_sensor_aggregate_status_t watch_sensor_aggregate_unavailable(void)
{
    return (watch_sensor_aggregate_status_t) {
        .state = WATCH_SENSOR_AGGREGATE_STATE_UNAVAILABLE,
    };
}

static void watch_sensor_aggregate_read_lsm6ds3(watch_sensor_aggregate_status_t *status)
{
    watch_lsm6ds3_service_status_t source;

    if (!watch_lsm6ds3_board_read_status(&source)) {
        *status = watch_sensor_aggregate_unavailable();
        return;
    }

    *status = (watch_sensor_aggregate_status_t) {
        .ready = source.ready,
        .sample_valid = source.sample_valid,
        .state = source.ready ? 1U : 0U,
        .last_sample_ms = source.last_sample_ms,
        .sample_count = source.sample_count,
        .error_count = source.read_error_count,
    };
}

static void watch_sensor_aggregate_read_lis2mdl(watch_sensor_aggregate_status_t *status)
{
    watch_lis2mdl_service_status_t source;

    if (!watch_lis2mdl_board_read_status(&source)) {
        *status = watch_sensor_aggregate_unavailable();
        return;
    }

    *status = (watch_sensor_aggregate_status_t) {
        .ready = source.ready,
        .sample_valid = source.sample_valid,
        .state = (uint8_t)source.state,
        .last_sample_ms = source.last_sample_ms,
        .sample_count = source.sample_count,
        .error_count = watch_sensor_aggregate_add_count(source.read_error_count, source.nack_count),
    };
}

static void watch_sensor_aggregate_read_aht20(watch_sensor_aggregate_status_t *status)
{
    watch_aht20_service_status_t source;

    if (!watch_aht20_board_read_status(&source)) {
        *status = watch_sensor_aggregate_unavailable();
        return;
    }

    *status = (watch_sensor_aggregate_status_t) {
        .ready = source.ready,
        .sample_valid = source.sample_valid,
        .state = (uint8_t)source.state,
        .last_sample_ms = source.last_sample_ms,
        .sample_count = source.sample_count,
        .error_count = watch_sensor_aggregate_add_count(
            watch_sensor_aggregate_add_count(source.read_error_count, source.crc_error_count),
            source.timeout_count),
    };
}

static void watch_sensor_aggregate_read_cw2015(watch_sensor_aggregate_status_t *status)
{
    watch_cw2015_service_status_t source;

    if (!watch_cw2015_board_read_status(&source)) {
        *status = watch_sensor_aggregate_unavailable();
        return;
    }

    *status = (watch_sensor_aggregate_status_t) {
        .ready = source.ready,
        .sample_valid = source.sample_valid,
        .state = (uint8_t)source.state,
        .last_sample_ms = source.last_sample_ms,
        .sample_count = source.sample_count,
        .error_count =
            watch_sensor_aggregate_add_count(source.read_error_count, source.invalid_soc_count),
    };
}

static void watch_sensor_aggregate_read_max30102(watch_sensor_aggregate_status_t *status)
{
    watch_max30102_service_status_t source;
    uint32_t errors;

    if (!watch_max30102_board_read_status(&source)) {
        *status = watch_sensor_aggregate_unavailable();
        return;
    }

    errors = watch_sensor_aggregate_add_count(source.read_error_count, source.id_error_count);
    errors = watch_sensor_aggregate_add_count(errors, source.reset_timeout_count);
    errors = watch_sensor_aggregate_add_count(errors, source.fifo_overflow_count);
    errors = watch_sensor_aggregate_add_count(errors, source.no_data_count);
    *status = (watch_sensor_aggregate_status_t) {
        .ready = source.ready,
        .sample_valid = source.sample_valid,
        .state = (uint8_t)source.state,
        .last_sample_ms = source.last_sample_ms,
        .sample_count = source.sample_count,
        .error_count = errors,
    };
}

static void watch_sensor_aggregate_board_init_once(void)
{
    if (s_service_initialized) {
        return;
    }

    s_service_initialized = watch_sensor_aggregate_init(&s_service);
}

void watch_sensor_aggregate_board_process(uint32_t now_ms)
{
    watch_sensor_aggregate_status_t statuses[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT];
    watch_sensor_aggregate_snapshot_t snapshot;
    watch_ui_event_t event;

    watch_sensor_aggregate_board_init_once();
    if (!s_service_initialized) {
        return;
    }

    watch_sensor_aggregate_read_lsm6ds3(&statuses[WATCH_SENSOR_AGGREGATE_LSM6DS3]);
    watch_sensor_aggregate_read_lis2mdl(&statuses[WATCH_SENSOR_AGGREGATE_LIS2MDL]);
    watch_sensor_aggregate_read_aht20(&statuses[WATCH_SENSOR_AGGREGATE_AHT20]);
    watch_sensor_aggregate_read_cw2015(&statuses[WATCH_SENSOR_AGGREGATE_CW2015]);
    watch_sensor_aggregate_read_max30102(&statuses[WATCH_SENSOR_AGGREGATE_MAX30102]);

    if (watch_sensor_aggregate_update(&s_service, statuses, &snapshot)) {
        s_event_pending = true;
    }

    if (!s_event_pending || !watch_sensor_aggregate_read_snapshot(&s_service, &snapshot)) {
        return;
    }

    event = (watch_ui_event_t) {
        .type = WATCH_SENSOR_AGGREGATE_SERVICE_EVENT_STATUS,
        .value = snapshot.available_mask,
        .timestamp_ms = now_ms,
        .sensor_snapshot = snapshot,
    };
    if (watch_runtime_post_ui_event(&event)) {
        s_event_pending = false;
    }
}

bool watch_sensor_aggregate_board_read_snapshot(watch_sensor_aggregate_snapshot_t *snapshot)
{
    return s_service_initialized && watch_sensor_aggregate_read_snapshot(&s_service, snapshot);
}
