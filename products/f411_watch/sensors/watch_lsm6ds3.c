#include "watch_lsm6ds3.h"

#include <stddef.h>

#define WATCH_LSM6DS3_REG_WHO_AM_I 0x0FU
#define WATCH_LSM6DS3_REG_CTRL1_XL 0x10U
#define WATCH_LSM6DS3_REG_CTRL2_G 0x11U
#define WATCH_LSM6DS3_REG_CTRL3_C 0x12U
#define WATCH_LSM6DS3_REG_OUTX_L_G 0x22U

#define WATCH_LSM6DS3_CTRL1_XL_104HZ_4G 0x48U
#define WATCH_LSM6DS3_CTRL2_G_104HZ_500DPS 0x44U
#define WATCH_LSM6DS3_CTRL3_C_BDU_IF_INC 0x44U

static bool watch_lsm6ds3_valid_bus(const watch_lsm6ds3_bus_t *bus)
{
    return bus != NULL && bus->read != NULL && bus->write != NULL;
}

static bool watch_lsm6ds3_read_register(const watch_lsm6ds3_t *device, uint8_t reg, uint8_t *data,
                                        uint8_t length)
{
    return device->bus.read(device->bus.context, WATCH_LSM6DS3_I2C_ADDRESS, reg, data, length);
}

static bool watch_lsm6ds3_write_register(const watch_lsm6ds3_t *device, uint8_t reg, uint8_t value)
{
    return device->bus.write(device->bus.context, WATCH_LSM6DS3_I2C_ADDRESS, reg, &value, 1U);
}

static int16_t watch_lsm6ds3_read_i16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[1] << 8U) | data[0]);
}

watch_lsm6ds3_result_t watch_lsm6ds3_init(watch_lsm6ds3_t *device, const watch_lsm6ds3_bus_t *bus)
{
    uint8_t who_am_i = 0U;

    if (device == NULL || !watch_lsm6ds3_valid_bus(bus)) {
        return WATCH_LSM6DS3_RESULT_INVALID_ARGUMENT;
    }

    *device = (watch_lsm6ds3_t) { 0 };
    device->bus = *bus;
    if (!watch_lsm6ds3_read_register(device, WATCH_LSM6DS3_REG_WHO_AM_I, &who_am_i, 1U)) {
        return WATCH_LSM6DS3_RESULT_BUS_ERROR;
    }

    device->who_am_i = who_am_i;
    if (who_am_i != WATCH_LSM6DS3_WHO_AM_I_VALUE) {
        return WATCH_LSM6DS3_RESULT_ID_MISMATCH;
    }

    if (!watch_lsm6ds3_write_register(device, WATCH_LSM6DS3_REG_CTRL1_XL,
                                      WATCH_LSM6DS3_CTRL1_XL_104HZ_4G)
        || !watch_lsm6ds3_write_register(device, WATCH_LSM6DS3_REG_CTRL2_G,
                                         WATCH_LSM6DS3_CTRL2_G_104HZ_500DPS)
        || !watch_lsm6ds3_write_register(device, WATCH_LSM6DS3_REG_CTRL3_C,
                                         WATCH_LSM6DS3_CTRL3_C_BDU_IF_INC)) {
        return WATCH_LSM6DS3_RESULT_BUS_ERROR;
    }

    device->initialized = true;
    return WATCH_LSM6DS3_RESULT_OK;
}

watch_lsm6ds3_result_t watch_lsm6ds3_read_sample(const watch_lsm6ds3_t *device,
                                                 watch_lsm6ds3_sample_t *sample)
{
    uint8_t data[12];

    if (device == NULL || sample == NULL) {
        return WATCH_LSM6DS3_RESULT_INVALID_ARGUMENT;
    }
    if (!device->initialized) {
        return WATCH_LSM6DS3_RESULT_NOT_READY;
    }
    if (!watch_lsm6ds3_read_register(device, WATCH_LSM6DS3_REG_OUTX_L_G, data, sizeof(data))) {
        return WATCH_LSM6DS3_RESULT_BUS_ERROR;
    }

    sample->gyro_x = watch_lsm6ds3_read_i16(&data[0]);
    sample->gyro_y = watch_lsm6ds3_read_i16(&data[2]);
    sample->gyro_z = watch_lsm6ds3_read_i16(&data[4]);
    sample->accel_x = watch_lsm6ds3_read_i16(&data[6]);
    sample->accel_y = watch_lsm6ds3_read_i16(&data[8]);
    sample->accel_z = watch_lsm6ds3_read_i16(&data[10]);
    return WATCH_LSM6DS3_RESULT_OK;
}

bool watch_lsm6ds3_service_init(watch_lsm6ds3_service_t *service, const watch_lsm6ds3_bus_t *bus,
                                watch_lsm6ds3_publish_fn publish, void *publish_context)
{
    watch_lsm6ds3_result_t result;

    if (service == NULL || !watch_lsm6ds3_valid_bus(bus)) {
        return false;
    }

    *service = (watch_lsm6ds3_service_t) { 0 };
    service->publish = publish;
    service->publish_context = publish_context;
    service->initialized = true;
    result = watch_lsm6ds3_init(&service->device, bus);
    service->ready = result == WATCH_LSM6DS3_RESULT_OK;
    return service->ready;
}

bool watch_lsm6ds3_service_process(watch_lsm6ds3_service_t *service, uint32_t now_ms)
{
    watch_lsm6ds3_sample_t sample;

    if (service == NULL || !service->initialized || !service->ready) {
        return false;
    }
    if (service->sample_valid
        && (now_ms - service->last_sample_ms) < WATCH_LSM6DS3_SERVICE_PERIOD_MS) {
        return false;
    }

    if (watch_lsm6ds3_read_sample(&service->device, &sample) != WATCH_LSM6DS3_RESULT_OK) {
        service->read_error_count++;
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

bool watch_lsm6ds3_service_read_status(const watch_lsm6ds3_service_t *service,
                                       watch_lsm6ds3_service_status_t *status)
{
    if (service == NULL || status == NULL || !service->initialized) {
        return false;
    }

    status->ready = service->ready;
    status->who_am_i = service->device.who_am_i;
    status->sample_valid = service->sample_valid;
    status->last_sample_ms = service->last_sample_ms;
    status->sample_count = service->sample_count;
    status->read_error_count = service->read_error_count;
    status->event_drop_count = service->event_drop_count;
    return true;
}

bool watch_lsm6ds3_service_read_latest(const watch_lsm6ds3_service_t *service,
                                       watch_lsm6ds3_sample_t *sample)
{
    if (service == NULL || sample == NULL || !service->initialized || !service->sample_valid) {
        return false;
    }

    *sample = service->latest_sample;
    return true;
}
