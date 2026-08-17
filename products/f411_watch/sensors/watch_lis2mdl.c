#include "watch_lis2mdl.h"

#include <stddef.h>

#define WATCH_LIS2MDL_REG_WHO_AM_I 0x4FU
#define WATCH_LIS2MDL_REG_CFG_A 0x60U
#define WATCH_LIS2MDL_REG_CFG_B 0x61U
#define WATCH_LIS2MDL_REG_CFG_C 0x62U
#define WATCH_LIS2MDL_REG_OUTX_L 0x68U

#define WATCH_LIS2MDL_CFG_A_CONTINUOUS_10HZ_TEMP_COMP 0x80U
#define WATCH_LIS2MDL_CFG_B_LOW_PASS_FILTER 0x01U
#define WATCH_LIS2MDL_CFG_C_BDU 0x10U

static int16_t watch_lis2mdl_read_i16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[1] << 8U) | data[0]);
}

static watch_lis2mdl_result_t
watch_lis2mdl_from_hub_result(watch_lsm6ds3_sensor_hub_result_t result)
{
    switch (result) {
    case WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK:
        return WATCH_LIS2MDL_RESULT_OK;
    case WATCH_LSM6DS3_SENSOR_HUB_RESULT_NACK:
        return WATCH_LIS2MDL_RESULT_NACK;
    case WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY:
        return WATCH_LIS2MDL_RESULT_NOT_READY;
    case WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT:
        return WATCH_LIS2MDL_RESULT_INVALID_ARGUMENT;
    case WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR:
    case WATCH_LSM6DS3_SENSOR_HUB_RESULT_COUNT:
        return WATCH_LIS2MDL_RESULT_BUS_ERROR;
    }

    return WATCH_LIS2MDL_RESULT_BUS_ERROR;
}

static void watch_lis2mdl_record_error(watch_lis2mdl_service_t *service,
                                       watch_lis2mdl_result_t result)
{
    if (result == WATCH_LIS2MDL_RESULT_NACK) {
        service->nack_count++;
    } else {
        service->read_error_count++;
    }
    service->ready = false;
    service->state = WATCH_LIS2MDL_SERVICE_STATE_FAILED;
}

static bool watch_lis2mdl_begin_write(watch_lis2mdl_service_t *service, uint8_t reg, uint8_t value,
                                      uint32_t now_ms, watch_lis2mdl_service_state_t next_state)
{
    watch_lis2mdl_result_t result =
        watch_lis2mdl_from_hub_result(watch_lsm6ds3_sensor_hub_configure_write(
            &service->hub, WATCH_LIS2MDL_I2C_ADDRESS, reg, value, now_ms));

    if (result != WATCH_LIS2MDL_RESULT_OK) {
        watch_lis2mdl_record_error(service, result);
        return false;
    }

    service->state = next_state;
    return true;
}

static bool watch_lis2mdl_begin_read(watch_lis2mdl_service_t *service, uint8_t reg, uint8_t length,
                                     uint32_t now_ms, watch_lis2mdl_service_state_t next_state)
{
    watch_lis2mdl_result_t result =
        watch_lis2mdl_from_hub_result(watch_lsm6ds3_sensor_hub_configure_read(
            &service->hub, WATCH_LIS2MDL_I2C_ADDRESS, reg, length, now_ms));

    if (result != WATCH_LIS2MDL_RESULT_OK) {
        watch_lis2mdl_record_error(service, result);
        return false;
    }

    service->state = next_state;
    return true;
}

static bool watch_lis2mdl_wait_for_hub(watch_lis2mdl_service_t *service, uint32_t now_ms)
{
    watch_lis2mdl_result_t result =
        watch_lis2mdl_from_hub_result(watch_lsm6ds3_sensor_hub_wait(&service->hub, now_ms));

    if (result == WATCH_LIS2MDL_RESULT_NOT_READY) {
        return false;
    }
    if (result != WATCH_LIS2MDL_RESULT_OK) {
        watch_lis2mdl_record_error(service, result);
        return false;
    }

    return true;
}

static bool watch_lis2mdl_read_cached_sample(watch_lis2mdl_service_t *service, uint32_t now_ms)
{
    uint8_t data[6];
    watch_lis2mdl_result_t result =
        watch_lis2mdl_from_hub_result(watch_lsm6ds3_sensor_hub_check_status(&service->hub));

    if (result == WATCH_LIS2MDL_RESULT_OK) {
        result = watch_lis2mdl_from_hub_result(
            watch_lsm6ds3_sensor_hub_read_cache(&service->hub, data, sizeof(data)));
    }
    if (result != WATCH_LIS2MDL_RESULT_OK
        || !watch_lis2mdl_decode_sample(data, &service->latest_sample)) {
        watch_lis2mdl_record_error(
            service, result == WATCH_LIS2MDL_RESULT_OK ? WATCH_LIS2MDL_RESULT_BUS_ERROR : result);
        return false;
    }

    service->last_sample_ms = now_ms;
    service->sample_count++;
    service->sample_valid = true;
    service->ready = true;
    if (service->publish != NULL && !service->publish(service->publish_context, now_ms)) {
        service->event_drop_count++;
    }
    return true;
}

bool watch_lis2mdl_validate_identity(uint8_t who_am_i)
{
    return who_am_i == WATCH_LIS2MDL_WHO_AM_I_VALUE;
}

bool watch_lis2mdl_decode_sample(const uint8_t data[6], watch_lis2mdl_sample_t *sample)
{
    if (data == NULL || sample == NULL) {
        return false;
    }

    sample->magnetic_x = watch_lis2mdl_read_i16(&data[0]);
    sample->magnetic_y = watch_lis2mdl_read_i16(&data[2]);
    sample->magnetic_z = watch_lis2mdl_read_i16(&data[4]);
    return true;
}

bool watch_lis2mdl_service_init(watch_lis2mdl_service_t *service, const watch_lsm6ds3_bus_t *bus,
                                watch_lis2mdl_publish_fn publish, void *publish_context)
{
    if (service == NULL) {
        return false;
    }

    *service = (watch_lis2mdl_service_t) { 0 };
    if (!watch_lsm6ds3_sensor_hub_init(&service->hub, bus)) {
        return false;
    }

    service->publish = publish;
    service->publish_context = publish_context;
    service->initialized = true;
    service->state = WATCH_LIS2MDL_SERVICE_STATE_NEW;
    return true;
}

bool watch_lis2mdl_service_process(watch_lis2mdl_service_t *service, uint32_t now_ms)
{
    uint8_t who_am_i;
    watch_lis2mdl_result_t result;

    if (service == NULL || !service->initialized) {
        return false;
    }

    switch (service->state) {
    case WATCH_LIS2MDL_SERVICE_STATE_NEW:
        return watch_lis2mdl_begin_read(service, WATCH_LIS2MDL_REG_WHO_AM_I, 1U, now_ms,
                                        WATCH_LIS2MDL_SERVICE_STATE_ID_WAIT);
    case WATCH_LIS2MDL_SERVICE_STATE_ID_WAIT:
        if (!watch_lis2mdl_wait_for_hub(service, now_ms)) {
            return false;
        }
        result = watch_lis2mdl_from_hub_result(
            watch_lsm6ds3_sensor_hub_read_cache(&service->hub, &who_am_i, 1U));
        if (result != WATCH_LIS2MDL_RESULT_OK) {
            watch_lis2mdl_record_error(service, result);
            return false;
        }
        service->who_am_i = who_am_i;
        if (!watch_lis2mdl_validate_identity(who_am_i)) {
            watch_lis2mdl_record_error(service, WATCH_LIS2MDL_RESULT_ID_MISMATCH);
            return false;
        }
        return watch_lis2mdl_begin_write(service, WATCH_LIS2MDL_REG_CFG_A,
                                         WATCH_LIS2MDL_CFG_A_CONTINUOUS_10HZ_TEMP_COMP, now_ms,
                                         WATCH_LIS2MDL_SERVICE_STATE_CONFIG_A_WAIT);
    case WATCH_LIS2MDL_SERVICE_STATE_CONFIG_A_WAIT:
        if (!watch_lis2mdl_wait_for_hub(service, now_ms)) {
            return false;
        }
        return watch_lis2mdl_begin_write(service, WATCH_LIS2MDL_REG_CFG_B,
                                         WATCH_LIS2MDL_CFG_B_LOW_PASS_FILTER, now_ms,
                                         WATCH_LIS2MDL_SERVICE_STATE_CONFIG_B_WAIT);
    case WATCH_LIS2MDL_SERVICE_STATE_CONFIG_B_WAIT:
        if (!watch_lis2mdl_wait_for_hub(service, now_ms)) {
            return false;
        }
        return watch_lis2mdl_begin_write(service, WATCH_LIS2MDL_REG_CFG_C, WATCH_LIS2MDL_CFG_C_BDU,
                                         now_ms, WATCH_LIS2MDL_SERVICE_STATE_CONFIG_C_WAIT);
    case WATCH_LIS2MDL_SERVICE_STATE_CONFIG_C_WAIT:
        if (!watch_lis2mdl_wait_for_hub(service, now_ms)) {
            return false;
        }
        return watch_lis2mdl_begin_read(service, WATCH_LIS2MDL_REG_OUTX_L, 6U, now_ms,
                                        WATCH_LIS2MDL_SERVICE_STATE_SAMPLE_WAIT);
    case WATCH_LIS2MDL_SERVICE_STATE_SAMPLE_WAIT:
        if (!watch_lis2mdl_wait_for_hub(service, now_ms)) {
            return false;
        }
        if (!watch_lis2mdl_read_cached_sample(service, now_ms)) {
            return false;
        }
        service->state = WATCH_LIS2MDL_SERVICE_STATE_READY;
        return true;
    case WATCH_LIS2MDL_SERVICE_STATE_READY:
        if ((now_ms - service->last_sample_ms) < WATCH_LIS2MDL_SERVICE_PERIOD_MS) {
            return false;
        }
        result = watch_lis2mdl_from_hub_result(
            watch_lsm6ds3_sensor_hub_start_read(&service->hub, now_ms));
        if (result != WATCH_LIS2MDL_RESULT_OK) {
            watch_lis2mdl_record_error(service, result);
            return false;
        }
        service->state = WATCH_LIS2MDL_SERVICE_STATE_SAMPLE_WAIT;
        return false;
    case WATCH_LIS2MDL_SERVICE_STATE_FAILED:
    case WATCH_LIS2MDL_SERVICE_STATE_COUNT:
        return false;
    }

    return false;
}

bool watch_lis2mdl_service_read_status(const watch_lis2mdl_service_t *service,
                                       watch_lis2mdl_service_status_t *status)
{
    if (service == NULL || status == NULL || !service->initialized) {
        return false;
    }

    *status = (watch_lis2mdl_service_status_t) {
        .ready = service->ready,
        .who_am_i = service->who_am_i,
        .sample_valid = service->sample_valid,
        .last_sample_ms = service->last_sample_ms,
        .sample_count = service->sample_count,
        .read_error_count = service->read_error_count,
        .nack_count = service->nack_count,
        .event_drop_count = service->event_drop_count,
        .state = service->state,
    };
    return true;
}

bool watch_lis2mdl_service_read_latest(const watch_lis2mdl_service_t *service,
                                       watch_lis2mdl_sample_t *sample)
{
    if (service == NULL || sample == NULL || !service->initialized || !service->sample_valid) {
        return false;
    }

    *sample = service->latest_sample;
    return true;
}
