#include "watch_max30102.h"

#include <stddef.h>

static bool watch_max30102_valid_bus(const watch_max30102_bus_t *bus)
{
    return bus != NULL && bus->read != NULL && bus->write != NULL;
}

static bool watch_max30102_deadline_expired(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static bool watch_max30102_read_byte(watch_max30102_service_t *service, uint8_t reg, uint8_t *value)
{
    return service->bus.read(service->bus.context, WATCH_MAX30102_I2C_ADDRESS, reg, value, 1U);
}

static bool watch_max30102_write_byte(watch_max30102_service_t *service, uint8_t reg, uint8_t value)
{
    return service->bus.write(service->bus.context, WATCH_MAX30102_I2C_ADDRESS, reg, &value, 1U);
}

bool watch_max30102_validate_identity(uint8_t part_id)
{
    return part_id == WATCH_MAX30102_PART_ID;
}

bool watch_max30102_decode_fifo_sample(const uint8_t data[6], watch_max30102_sample_t *sample)
{
    if (data == NULL || sample == NULL) {
        return false;
    }

    sample->red_raw = ((uint32_t)(data[0] & 0x03U) << 16U) | ((uint32_t)data[1] << 8U) | data[2];
    sample->ir_raw = ((uint32_t)(data[3] & 0x03U) << 16U) | ((uint32_t)data[4] << 8U) | data[5];
    sample->finger_on = sample->ir_raw > WATCH_MAX30102_FINGER_THRESHOLD;
    return true;
}

static bool watch_max30102_fail_read(watch_max30102_service_t *service)
{
    service->read_error_count++;
    service->state = WATCH_MAX30102_SERVICE_STATE_FAILED;
    return false;
}

static bool watch_max30102_write_config(watch_max30102_service_t *service, uint8_t reg,
                                        uint8_t value, watch_max30102_service_state_t next_state)
{
    if (!watch_max30102_write_byte(service, reg, value)) {
        return watch_max30102_fail_read(service);
    }

    service->state = next_state;
    return true;
}

static bool watch_max30102_process_ready(watch_max30102_service_t *service, uint32_t now_ms)
{
    uint8_t fifo_overflow;
    uint8_t fifo_write_pointer;
    uint8_t fifo_read_pointer;
    uint8_t fifo_data[6];
    uint8_t fifo_count;
    watch_max30102_sample_t sample;

    if (service->sample_valid
        && (now_ms - service->last_sample_ms) < WATCH_MAX30102_SERVICE_PERIOD_MS) {
        return false;
    }

    if (!watch_max30102_read_byte(service, WATCH_MAX30102_REG_FIFO_WR_PTR, &fifo_write_pointer)
        || !watch_max30102_read_byte(service, WATCH_MAX30102_REG_FIFO_RD_PTR, &fifo_read_pointer)
        || !watch_max30102_read_byte(service, WATCH_MAX30102_REG_FIFO_OVF_COUNTER,
                                     &fifo_overflow)) {
        return watch_max30102_fail_read(service);
    }

    service->fifo_write_pointer = fifo_write_pointer & (WATCH_MAX30102_FIFO_DEPTH - 1U);
    service->fifo_read_pointer = fifo_read_pointer & (WATCH_MAX30102_FIFO_DEPTH - 1U);
    service->fifo_overflow_count += fifo_overflow;
    fifo_count = (uint8_t)((service->fifo_write_pointer - service->fifo_read_pointer)
                           & (WATCH_MAX30102_FIFO_DEPTH - 1U));
    if (fifo_count == 0U) {
        service->no_data_count++;
        return false;
    }

    if (!service->bus.read(service->bus.context, WATCH_MAX30102_I2C_ADDRESS,
                           WATCH_MAX30102_REG_FIFO_DATA, fifo_data, sizeof(fifo_data))) {
        return watch_max30102_fail_read(service);
    }
    if (!watch_max30102_decode_fifo_sample(fifo_data, &sample)) {
        return watch_max30102_fail_read(service);
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

bool watch_max30102_service_init(watch_max30102_service_t *service, const watch_max30102_bus_t *bus,
                                 watch_max30102_publish_fn publish, void *publish_context)
{
    if (service == NULL || !watch_max30102_valid_bus(bus)) {
        return false;
    }

    *service = (watch_max30102_service_t) { 0 };
    service->bus = *bus;
    service->publish = publish;
    service->publish_context = publish_context;
    service->initialized = true;
    service->state = WATCH_MAX30102_SERVICE_STATE_NEW;
    return true;
}

bool watch_max30102_service_process(watch_max30102_service_t *service, uint32_t now_ms)
{
    uint8_t value;
    uint8_t part_id;
    uint8_t revision_id;

    if (service == NULL || !service->initialized
        || service->state == WATCH_MAX30102_SERVICE_STATE_FAILED) {
        return false;
    }

    switch (service->state) {
    case WATCH_MAX30102_SERVICE_STATE_NEW:
        if (!watch_max30102_read_byte(service, WATCH_MAX30102_REG_PART_ID, &part_id)
            || !watch_max30102_read_byte(service, WATCH_MAX30102_REG_REV_ID, &revision_id)) {
            return watch_max30102_fail_read(service);
        }
        service->part_id = part_id;
        service->revision_id = revision_id;
        if (!watch_max30102_validate_identity(part_id)) {
            service->id_error_count++;
            service->state = WATCH_MAX30102_SERVICE_STATE_FAILED;
            return false;
        }
        if (!watch_max30102_write_byte(service, WATCH_MAX30102_REG_MODE_CONFIG,
                                       WATCH_MAX30102_MODE_RESET)) {
            return watch_max30102_fail_read(service);
        }
        service->deadline_ms = now_ms + WATCH_MAX30102_RESET_TIMEOUT_MS;
        service->state = WATCH_MAX30102_SERVICE_STATE_RESET_WAIT;
        return true;
    case WATCH_MAX30102_SERVICE_STATE_RESET_WAIT:
        if (!watch_max30102_read_byte(service, WATCH_MAX30102_REG_MODE_CONFIG, &value)) {
            return watch_max30102_fail_read(service);
        }
        if ((value & WATCH_MAX30102_MODE_RESET) != 0U) {
            if (watch_max30102_deadline_expired(now_ms, service->deadline_ms)) {
                service->reset_timeout_count++;
                service->state = WATCH_MAX30102_SERVICE_STATE_FAILED;
            }
            return false;
        }
        service->mode_config = value;
        service->state = WATCH_MAX30102_SERVICE_STATE_CLEAR_WRITE_PTR;
        return true;
    case WATCH_MAX30102_SERVICE_STATE_CLEAR_WRITE_PTR:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_FIFO_WR_PTR, 0U,
                                           WATCH_MAX30102_SERVICE_STATE_CLEAR_OVERFLOW);
    case WATCH_MAX30102_SERVICE_STATE_CLEAR_OVERFLOW:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_FIFO_OVF_COUNTER, 0U,
                                           WATCH_MAX30102_SERVICE_STATE_CLEAR_READ_PTR);
    case WATCH_MAX30102_SERVICE_STATE_CLEAR_READ_PTR:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_FIFO_RD_PTR, 0U,
                                           WATCH_MAX30102_SERVICE_STATE_CONFIG_FIFO);
    case WATCH_MAX30102_SERVICE_STATE_CONFIG_FIFO:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_FIFO_CONFIG, 0x50U,
                                           WATCH_MAX30102_SERVICE_STATE_CONFIG_SPO2);
    case WATCH_MAX30102_SERVICE_STATE_CONFIG_SPO2:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_SPO2_CONFIG, 0x27U,
                                           WATCH_MAX30102_SERVICE_STATE_CONFIG_RED);
    case WATCH_MAX30102_SERVICE_STATE_CONFIG_RED:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_LED1_PA, 0x1FU,
                                           WATCH_MAX30102_SERVICE_STATE_CONFIG_IR);
    case WATCH_MAX30102_SERVICE_STATE_CONFIG_IR:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_LED2_PA, 0x1FU,
                                           WATCH_MAX30102_SERVICE_STATE_CONFIG_INTERRUPT);
    case WATCH_MAX30102_SERVICE_STATE_CONFIG_INTERRUPT:
        return watch_max30102_write_config(service, WATCH_MAX30102_REG_INT_ENABLE1, 0x80U,
                                           WATCH_MAX30102_SERVICE_STATE_CONFIG_MODE);
    case WATCH_MAX30102_SERVICE_STATE_CONFIG_MODE:
        if (!watch_max30102_write_byte(service, WATCH_MAX30102_REG_MODE_CONFIG,
                                       WATCH_MAX30102_MODE_SPO2)) {
            return watch_max30102_fail_read(service);
        }
        service->mode_config = WATCH_MAX30102_MODE_SPO2;
        service->ready = true;
        service->state = WATCH_MAX30102_SERVICE_STATE_READY;
        return true;
    case WATCH_MAX30102_SERVICE_STATE_READY:
        return watch_max30102_process_ready(service, now_ms);
    case WATCH_MAX30102_SERVICE_STATE_FAILED:
    case WATCH_MAX30102_SERVICE_STATE_COUNT:
        return false;
    }

    return false;
}

bool watch_max30102_service_read_status(const watch_max30102_service_t *service,
                                        watch_max30102_service_status_t *status)
{
    if (service == NULL || status == NULL || !service->initialized) {
        return false;
    }

    *status = (watch_max30102_service_status_t) {
        .ready = service->ready,
        .part_id = service->part_id,
        .revision_id = service->revision_id,
        .mode_config = service->mode_config,
        .sample_valid = service->sample_valid,
        .last_sample_ms = service->last_sample_ms,
        .sample_count = service->sample_count,
        .no_data_count = service->no_data_count,
        .read_error_count = service->read_error_count,
        .id_error_count = service->id_error_count,
        .reset_timeout_count = service->reset_timeout_count,
        .fifo_overflow_count = service->fifo_overflow_count,
        .event_drop_count = service->event_drop_count,
        .state = service->state,
    };
    return true;
}

bool watch_max30102_service_read_latest(const watch_max30102_service_t *service,
                                        watch_max30102_sample_t *sample)
{
    if (service == NULL || sample == NULL || !service->initialized || !service->sample_valid) {
        return false;
    }

    *sample = service->latest_sample;
    return true;
}
