#include "watch_aht20.h"

#include <stddef.h>

static const uint8_t s_initialize_command[] = { WATCH_AHT20_COMMAND_INITIALIZE, 0x08U, 0x00U };
static const uint8_t s_trigger_command[] = { WATCH_AHT20_COMMAND_TRIGGER, 0x33U, 0x00U };

static bool watch_aht20_valid_bus(const watch_aht20_bus_t *bus)
{
    return bus != NULL && bus->write != NULL && bus->read != NULL;
}

static bool watch_aht20_deadline_expired(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static uint8_t watch_aht20_crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFFU;

    for (uint8_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U ? (uint8_t)((crc << 1U) ^ 0x31U) : (uint8_t)(crc << 1U);
        }
    }

    return crc;
}

bool watch_aht20_validate_crc(const uint8_t data[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH])
{
    if (data == NULL) {
        return false;
    }

    return watch_aht20_crc8(data, WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH - 1U)
        == data[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH - 1U];
}

bool watch_aht20_decode_measurement(const uint8_t data[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH],
                                    watch_aht20_sample_t *sample)
{
    uint32_t raw_humidity;
    uint32_t raw_temperature;
    int32_t temperature_centi_c;

    if (data == NULL || sample == NULL || (data[0] & WATCH_AHT20_STATUS_BUSY) != 0U
        || !watch_aht20_validate_crc(data)) {
        return false;
    }

    raw_humidity =
        ((uint32_t)data[1] << 12U) | ((uint32_t)data[2] << 4U) | ((uint32_t)data[3] >> 4U);
    raw_temperature = ((uint32_t)(data[3] & 0x0FU) << 16U) | ((uint32_t)data[4] << 8U) | data[5];

    sample->humidity_centi_percent =
        (uint16_t)((((uint64_t)raw_humidity * 10000ULL) + (1ULL << 19U)) >> 20U);
    if (sample->humidity_centi_percent > 10000U) {
        sample->humidity_centi_percent = 10000U;
    }

    temperature_centi_c =
        (int32_t)((((uint64_t)raw_temperature * 20000ULL) + (1ULL << 19U)) >> 20U) - 5000;
    if (temperature_centi_c < INT16_MIN) {
        temperature_centi_c = INT16_MIN;
    } else if (temperature_centi_c > INT16_MAX) {
        temperature_centi_c = INT16_MAX;
    }
    sample->temperature_centi_c = (int16_t)temperature_centi_c;
    return true;
}

static void watch_aht20_record_error(watch_aht20_service_t *service, watch_aht20_result_t result)
{
    switch (result) {
    case WATCH_AHT20_RESULT_CRC_ERROR:
        service->crc_error_count++;
        break;
    case WATCH_AHT20_RESULT_BUSY_TIMEOUT:
        service->timeout_count++;
        break;
    case WATCH_AHT20_RESULT_BUS_ERROR:
    case WATCH_AHT20_RESULT_DATA_ERROR:
    case WATCH_AHT20_RESULT_INVALID_ARGUMENT:
    case WATCH_AHT20_RESULT_NOT_READY:
    case WATCH_AHT20_RESULT_OK:
    case WATCH_AHT20_RESULT_COUNT:
        service->read_error_count++;
        break;
    }
}

static bool watch_aht20_write_command(const watch_aht20_service_t *service, const uint8_t *command,
                                      uint8_t length)
{
    return service->bus.write(service->bus.context, WATCH_AHT20_I2C_ADDRESS, command, length);
}

static bool watch_aht20_read_status(watch_aht20_service_t *service)
{
    return service->bus.read(service->bus.context, WATCH_AHT20_I2C_ADDRESS, &service->status_byte,
                             1U);
}

static bool watch_aht20_start_measurement(watch_aht20_service_t *service, uint32_t now_ms)
{
    if (!watch_aht20_write_command(service, s_trigger_command, sizeof(s_trigger_command))) {
        watch_aht20_record_error(service, WATCH_AHT20_RESULT_BUS_ERROR);
        return false;
    }

    service->deadline_ms = now_ms + WATCH_AHT20_MEASUREMENT_TIMEOUT_MS;
    service->state = WATCH_AHT20_SERVICE_STATE_MEASUREMENT_WAIT;
    return true;
}

static bool watch_aht20_finish_measurement(watch_aht20_service_t *service, uint32_t now_ms)
{
    uint8_t data[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH];
    watch_aht20_sample_t sample;

    if (!service->bus.read(service->bus.context, WATCH_AHT20_I2C_ADDRESS, data, sizeof(data))) {
        watch_aht20_record_error(service, WATCH_AHT20_RESULT_BUS_ERROR);
        service->state = WATCH_AHT20_SERVICE_STATE_READY;
        return false;
    }
    service->status_byte = data[0];
    if ((data[0] & WATCH_AHT20_STATUS_BUSY) != 0U) {
        if (watch_aht20_deadline_expired(now_ms, service->deadline_ms)) {
            watch_aht20_record_error(service, WATCH_AHT20_RESULT_BUSY_TIMEOUT);
            service->state = WATCH_AHT20_SERVICE_STATE_READY;
        }
        return false;
    }
    if (!watch_aht20_validate_crc(data)) {
        watch_aht20_record_error(service, WATCH_AHT20_RESULT_CRC_ERROR);
        service->state = WATCH_AHT20_SERVICE_STATE_READY;
        return false;
    }
    if (!watch_aht20_decode_measurement(data, &sample)) {
        watch_aht20_record_error(service, WATCH_AHT20_RESULT_DATA_ERROR);
        service->state = WATCH_AHT20_SERVICE_STATE_READY;
        return false;
    }

    service->latest_sample = sample;
    service->last_sample_ms = now_ms;
    service->sample_count++;
    service->sample_valid = true;
    service->state = WATCH_AHT20_SERVICE_STATE_READY;
    if (service->publish != NULL && !service->publish(service->publish_context, now_ms)) {
        service->event_drop_count++;
    }
    return true;
}

bool watch_aht20_service_init(watch_aht20_service_t *service, const watch_aht20_bus_t *bus,
                              watch_aht20_publish_fn publish, void *publish_context)
{
    if (service == NULL || !watch_aht20_valid_bus(bus)) {
        return false;
    }

    *service = (watch_aht20_service_t) { 0 };
    service->bus = *bus;
    service->publish = publish;
    service->publish_context = publish_context;
    service->initialized = true;
    service->state = WATCH_AHT20_SERVICE_STATE_NEW;
    return true;
}

bool watch_aht20_service_process(watch_aht20_service_t *service, uint32_t now_ms)
{
    if (service == NULL || !service->initialized
        || service->state == WATCH_AHT20_SERVICE_STATE_FAILED) {
        return false;
    }

    switch (service->state) {
    case WATCH_AHT20_SERVICE_STATE_NEW:
        if (!watch_aht20_write_command(service, s_initialize_command,
                                       sizeof(s_initialize_command))) {
            watch_aht20_record_error(service, WATCH_AHT20_RESULT_BUS_ERROR);
            service->state = WATCH_AHT20_SERVICE_STATE_FAILED;
            return false;
        }
        service->deadline_ms = now_ms + WATCH_AHT20_INIT_TIMEOUT_MS;
        service->state = WATCH_AHT20_SERVICE_STATE_INIT_WAIT;
        return true;
    case WATCH_AHT20_SERVICE_STATE_INIT_WAIT:
        if (!watch_aht20_read_status(service)) {
            watch_aht20_record_error(service, WATCH_AHT20_RESULT_BUS_ERROR);
            service->state = WATCH_AHT20_SERVICE_STATE_FAILED;
            return false;
        }
        service->calibrated = (service->status_byte & WATCH_AHT20_STATUS_CALIBRATED) != 0U;
        if ((service->status_byte & WATCH_AHT20_STATUS_BUSY) != 0U) {
            if (watch_aht20_deadline_expired(now_ms, service->deadline_ms)) {
                watch_aht20_record_error(service, WATCH_AHT20_RESULT_BUSY_TIMEOUT);
                service->state = WATCH_AHT20_SERVICE_STATE_FAILED;
            }
            return false;
        }
        service->ready = true;
        return watch_aht20_start_measurement(service, now_ms);
    case WATCH_AHT20_SERVICE_STATE_MEASUREMENT_WAIT:
        return watch_aht20_finish_measurement(service, now_ms);
    case WATCH_AHT20_SERVICE_STATE_READY:
        if (!service->sample_valid
            || (now_ms - service->last_sample_ms) >= WATCH_AHT20_SERVICE_PERIOD_MS) {
            return watch_aht20_start_measurement(service, now_ms);
        }
        return false;
    case WATCH_AHT20_SERVICE_STATE_FAILED:
    case WATCH_AHT20_SERVICE_STATE_COUNT:
        return false;
    }

    return false;
}

bool watch_aht20_service_read_status(const watch_aht20_service_t *service,
                                     watch_aht20_service_status_t *status)
{
    if (service == NULL || status == NULL || !service->initialized) {
        return false;
    }

    *status = (watch_aht20_service_status_t) {
        .ready = service->ready,
        .calibrated = service->calibrated,
        .sample_valid = service->sample_valid,
        .status_byte = service->status_byte,
        .last_sample_ms = service->last_sample_ms,
        .sample_count = service->sample_count,
        .read_error_count = service->read_error_count,
        .crc_error_count = service->crc_error_count,
        .timeout_count = service->timeout_count,
        .event_drop_count = service->event_drop_count,
        .state = service->state,
    };
    return true;
}

bool watch_aht20_service_read_latest(const watch_aht20_service_t *service,
                                     watch_aht20_sample_t *sample)
{
    if (service == NULL || sample == NULL || !service->initialized || !service->sample_valid) {
        return false;
    }

    *sample = service->latest_sample;
    return true;
}
