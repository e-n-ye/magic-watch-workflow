#include "watch_lsm6ds3_sensor_hub.h"

#include <stddef.h>

#define WATCH_LSM6DS3_REG_FUNC_CFG_ACCESS 0x01U
#define WATCH_LSM6DS3_REG_MASTER_CONFIG 0x1AU
#define WATCH_LSM6DS3_REG_SENSORHUB1 0x2EU
#define WATCH_LSM6DS3_REG_FUNC_SRC 0x53U
#define WATCH_LSM6DS3_REG_FUNC_SRC2 0x54U

#define WATCH_LSM6DS3_EMBEDDED_BANK_A 0x80U
#define WATCH_LSM6DS3_USER_BANK 0x00U
#define WATCH_LSM6DS3_EMBEDDED_REG_SLV0_ADD 0x02U
#define WATCH_LSM6DS3_EMBEDDED_REG_SLV0_SUBADD 0x03U
#define WATCH_LSM6DS3_EMBEDDED_REG_SLAVE0_CONFIG 0x04U
#define WATCH_LSM6DS3_EMBEDDED_REG_DATAWRITE_SLV0 0x0EU

#define WATCH_LSM6DS3_MASTER_ON 0x01U
#define WATCH_LSM6DS3_PULL_UP_ENABLE 0x08U
#define WATCH_LSM6DS3_SLV0_READ 0x01U
#define WATCH_LSM6DS3_SLV0_NACK 0x08U
#define WATCH_LSM6DS3_SENSOR_HUB_END_OP 0x01U
#define WATCH_LSM6DS3_SLV0_READ_LENGTH_MASK 0x07U
#define WATCH_LSM6DS3_SENSOR_HUB_MAX_READ_LENGTH 7U

static bool watch_lsm6ds3_sensor_hub_valid(const watch_lsm6ds3_sensor_hub_t *hub)
{
    return hub != NULL && hub->initialized;
}

static bool watch_lsm6ds3_sensor_hub_read_register(const watch_lsm6ds3_sensor_hub_t *hub,
                                                   uint8_t reg, uint8_t *data, uint8_t length)
{
    return hub->bus.read(hub->bus.context, WATCH_LSM6DS3_I2C_ADDRESS, reg, data, length);
}

static bool watch_lsm6ds3_sensor_hub_write_register(const watch_lsm6ds3_sensor_hub_t *hub,
                                                    uint8_t reg, uint8_t value)
{
    return hub->bus.write(hub->bus.context, WATCH_LSM6DS3_I2C_ADDRESS, reg, &value, 1U);
}

static bool watch_lsm6ds3_sensor_hub_select_bank(const watch_lsm6ds3_sensor_hub_t *hub,
                                                 uint8_t bank)
{
    return watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_REG_FUNC_CFG_ACCESS, bank);
}

static bool watch_lsm6ds3_sensor_hub_finish_embedded_access(const watch_lsm6ds3_sensor_hub_t *hub,
                                                            bool success)
{
    return watch_lsm6ds3_sensor_hub_select_bank(hub, WATCH_LSM6DS3_USER_BANK) && success;
}

static bool watch_lsm6ds3_sensor_hub_disable_master(const watch_lsm6ds3_sensor_hub_t *hub)
{
    return watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_REG_MASTER_CONFIG, 0U);
}

static bool watch_lsm6ds3_sensor_hub_enable_master(const watch_lsm6ds3_sensor_hub_t *hub)
{
    return watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_REG_MASTER_CONFIG,
                                                   WATCH_LSM6DS3_MASTER_ON
                                                       | WATCH_LSM6DS3_PULL_UP_ENABLE);
}

static watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_begin_operation(watch_lsm6ds3_sensor_hub_t *hub, uint32_t now_ms)
{
    if (!watch_lsm6ds3_sensor_hub_enable_master(hub)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    hub->operation_started_ms = now_ms;
    hub->operation_pending = true;
    return WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK;
}

bool watch_lsm6ds3_sensor_hub_init(watch_lsm6ds3_sensor_hub_t *hub, const watch_lsm6ds3_bus_t *bus)
{
    if (hub == NULL || bus == NULL || bus->read == NULL || bus->write == NULL) {
        return false;
    }

    *hub = (watch_lsm6ds3_sensor_hub_t) { 0 };
    hub->bus = *bus;
    hub->initialized = true;
    return true;
}

watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_configure_read(watch_lsm6ds3_sensor_hub_t *hub, uint8_t slave_address,
                                        uint8_t reg, uint8_t length, uint32_t now_ms)
{
    bool success;

    if (!watch_lsm6ds3_sensor_hub_valid(hub) || length == 0U
        || length > WATCH_LSM6DS3_SENSOR_HUB_MAX_READ_LENGTH) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT;
    }
    if (!watch_lsm6ds3_sensor_hub_disable_master(hub)
        || !watch_lsm6ds3_sensor_hub_select_bank(hub, WATCH_LSM6DS3_EMBEDDED_BANK_A)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    success = watch_lsm6ds3_sensor_hub_write_register(
                  hub, WATCH_LSM6DS3_EMBEDDED_REG_SLV0_ADD,
                  (uint8_t)((slave_address << 1U) | WATCH_LSM6DS3_SLV0_READ))
        && watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_EMBEDDED_REG_SLV0_SUBADD, reg)
        && watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_EMBEDDED_REG_SLAVE0_CONFIG,
                                                   length & WATCH_LSM6DS3_SLV0_READ_LENGTH_MASK);
    if (!watch_lsm6ds3_sensor_hub_finish_embedded_access(hub, success)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    return watch_lsm6ds3_sensor_hub_begin_operation(hub, now_ms);
}

watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_configure_write(watch_lsm6ds3_sensor_hub_t *hub, uint8_t slave_address,
                                         uint8_t reg, uint8_t value, uint32_t now_ms)
{
    bool success;

    if (!watch_lsm6ds3_sensor_hub_valid(hub)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT;
    }
    if (!watch_lsm6ds3_sensor_hub_disable_master(hub)
        || !watch_lsm6ds3_sensor_hub_select_bank(hub, WATCH_LSM6DS3_EMBEDDED_BANK_A)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    success = watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_EMBEDDED_REG_SLV0_ADD,
                                                      (uint8_t)(slave_address << 1U))
        && watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_EMBEDDED_REG_SLV0_SUBADD, reg)
        && watch_lsm6ds3_sensor_hub_write_register(hub, WATCH_LSM6DS3_EMBEDDED_REG_DATAWRITE_SLV0,
                                                   value);
    if (!watch_lsm6ds3_sensor_hub_finish_embedded_access(hub, success)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    return watch_lsm6ds3_sensor_hub_begin_operation(hub, now_ms);
}

watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_start_read(watch_lsm6ds3_sensor_hub_t *hub, uint32_t now_ms)
{
    if (!watch_lsm6ds3_sensor_hub_valid(hub)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT;
    }

    return watch_lsm6ds3_sensor_hub_begin_operation(hub, now_ms);
}

watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_check_status(const watch_lsm6ds3_sensor_hub_t *hub)
{
    uint8_t status;

    if (!watch_lsm6ds3_sensor_hub_valid(hub)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT;
    }
    if (!watch_lsm6ds3_sensor_hub_read_register(hub, WATCH_LSM6DS3_REG_FUNC_SRC2, &status, 1U)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    return (status & WATCH_LSM6DS3_SLV0_NACK) == 0U ? WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK
                                                    : WATCH_LSM6DS3_SENSOR_HUB_RESULT_NACK;
}

watch_lsm6ds3_sensor_hub_result_t watch_lsm6ds3_sensor_hub_wait(watch_lsm6ds3_sensor_hub_t *hub,
                                                                uint32_t now_ms)
{
    watch_lsm6ds3_sensor_hub_result_t result;
    uint8_t status;

    if (!watch_lsm6ds3_sensor_hub_valid(hub)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT;
    }
    if (!hub->operation_pending) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY;
    }
    if ((now_ms - hub->operation_started_ms) < WATCH_LSM6DS3_SENSOR_HUB_CYCLE_WAIT_MS) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY;
    }

    if (!watch_lsm6ds3_sensor_hub_read_register(hub, WATCH_LSM6DS3_REG_FUNC_SRC, &status, 1U)) {
        hub->operation_pending = false;
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }
    if ((status & WATCH_LSM6DS3_SENSOR_HUB_END_OP) == 0U) {
        if ((now_ms - hub->operation_started_ms) < WATCH_LSM6DS3_SENSOR_HUB_CYCLE_TIMEOUT_MS) {
            return WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY;
        }

        hub->operation_pending = false;
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    result = watch_lsm6ds3_sensor_hub_check_status(hub);
    if (result != WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK) {
        hub->operation_pending = false;
        return result;
    }

    hub->operation_pending = false;
    return WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK;
}

watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_read_cache(const watch_lsm6ds3_sensor_hub_t *hub, uint8_t *data,
                                    uint8_t length)
{
    if (!watch_lsm6ds3_sensor_hub_valid(hub) || data == NULL || length == 0U
        || length > WATCH_LSM6DS3_SENSOR_HUB_MAX_READ_LENGTH) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT;
    }
    if (!watch_lsm6ds3_sensor_hub_read_register(hub, WATCH_LSM6DS3_REG_SENSORHUB1, data, length)) {
        return WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR;
    }

    return WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK;
}
