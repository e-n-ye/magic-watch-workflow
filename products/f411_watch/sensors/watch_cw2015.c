#include "watch_cw2015.h"

#include <stddef.h>

static bool watch_cw2015_valid_bus(const watch_cw2015_bus_t *bus)
{
    return bus != NULL && bus->read != NULL;
}

static bool watch_cw2015_read_register(watch_cw2015_service_t *service, uint8_t reg, uint8_t *data,
                                       uint8_t length)
{
    return service->bus.read(service->bus.context, WATCH_CW2015_I2C_ADDRESS, reg, data, length);
}

bool watch_cw2015_decode_voltage(const uint8_t data[2], uint16_t *voltage_mv)
{
    uint32_t raw_voltage;
    uint32_t voltage;

    if (data == NULL || voltage_mv == NULL) {
        return false;
    }

    raw_voltage = ((uint32_t)data[0] << 8U) | data[1];
    voltage = (raw_voltage * 305U) / 1000U;
    if (voltage > UINT16_MAX) {
        voltage = UINT16_MAX;
    }
    *voltage_mv = (uint16_t)voltage;
    return true;
}

bool watch_cw2015_decode_soc(const uint8_t data[2], uint8_t *soc_percent, uint8_t *soc_fraction)
{
    if (data == NULL || soc_percent == NULL || soc_fraction == NULL
        || data[0] == WATCH_CW2015_SOC_INVALID || data[0] > 100U) {
        return false;
    }

    *soc_percent = data[0];
    *soc_fraction = data[1];
    return true;
}

bool watch_cw2015_service_init(watch_cw2015_service_t *service, const watch_cw2015_bus_t *bus,
                               watch_cw2015_publish_fn publish, void *publish_context)
{
    if (service == NULL || !watch_cw2015_valid_bus(bus)) {
        return false;
    }

    *service = (watch_cw2015_service_t) { 0 };
    service->bus = *bus;
    service->publish = publish;
    service->publish_context = publish_context;
    service->initialized = true;
    service->state = WATCH_CW2015_SERVICE_STATE_NEW;
    return true;
}

bool watch_cw2015_service_process(watch_cw2015_service_t *service, uint32_t now_ms)
{
    uint8_t version;
    uint8_t voltage_data[2];
    uint8_t soc_data[2];
    watch_cw2015_sample_t sample;

    if (service == NULL || !service->initialized
        || service->state == WATCH_CW2015_SERVICE_STATE_FAILED) {
        return false;
    }

    if (service->state == WATCH_CW2015_SERVICE_STATE_NEW) {
        if (!watch_cw2015_read_register(service, WATCH_CW2015_REG_VERSION, &version, 1U)) {
            service->read_error_count++;
            service->state = WATCH_CW2015_SERVICE_STATE_FAILED;
            return false;
        }
        service->version = version;
        service->ready = true;
        service->state = WATCH_CW2015_SERVICE_STATE_READY;
    }

    if (service->sample_valid
        && (now_ms - service->last_sample_ms) < WATCH_CW2015_SERVICE_PERIOD_MS) {
        return false;
    }

    if (!watch_cw2015_read_register(service, WATCH_CW2015_REG_VCELL, voltage_data,
                                    sizeof(voltage_data))
        || !watch_cw2015_read_register(service, WATCH_CW2015_REG_SOC, soc_data, sizeof(soc_data))) {
        service->read_error_count++;
        return false;
    }

    if (!watch_cw2015_decode_voltage(voltage_data, &sample.voltage_mv)
        || !watch_cw2015_decode_soc(soc_data, &sample.soc_percent, &sample.soc_fraction)) {
        service->invalid_soc_count++;
        return false;
    }

    service->latest_sample = sample;
    service->last_sample_ms = now_ms;
    service->sample_count++;
    service->sample_valid = true;
    if (service->publish != NULL && !service->publish(service->publish_context, now_ms)) {
        service->event_drop_count++;
    }
    return true;
}

bool watch_cw2015_service_read_status(const watch_cw2015_service_t *service,
                                      watch_cw2015_service_status_t *status)
{
    if (service == NULL || status == NULL || !service->initialized) {
        return false;
    }

    *status = (watch_cw2015_service_status_t) {
        .ready = service->ready,
        .sample_valid = service->sample_valid,
        .version = service->version,
        .soc_percent = service->sample_valid ? service->latest_sample.soc_percent : 0U,
        .soc_fraction = service->sample_valid ? service->latest_sample.soc_fraction : 0U,
        .voltage_mv = service->sample_valid ? service->latest_sample.voltage_mv : 0U,
        .last_sample_ms = service->last_sample_ms,
        .sample_count = service->sample_count,
        .read_error_count = service->read_error_count,
        .invalid_soc_count = service->invalid_soc_count,
        .event_drop_count = service->event_drop_count,
        .state = service->state,
    };
    return true;
}

bool watch_cw2015_service_read_latest(const watch_cw2015_service_t *service,
                                      watch_cw2015_sample_t *sample)
{
    if (service == NULL || sample == NULL || !service->initialized || !service->sample_valid) {
        return false;
    }

    *sample = service->latest_sample;
    return true;
}
