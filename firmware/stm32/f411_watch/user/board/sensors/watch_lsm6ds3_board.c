#include "board/sensors/watch_lsm6ds3_board.h"

#include "i2c.h"
#include "main.h"
#include "watch_runtime.h"

#define WATCH_LSM6DS3_I2C_TIMEOUT_MS 100U

static watch_lsm6ds3_service_t s_service;
static bool s_service_initialized;

static bool watch_lsm6ds3_i2c_read(void *context, uint8_t address, uint8_t reg, uint8_t *data,
                                   uint8_t length)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_Mem_Read(handle, (uint16_t)(address << 1U), reg, I2C_MEMADD_SIZE_8BIT, data,
                            length, WATCH_LSM6DS3_I2C_TIMEOUT_MS)
        == HAL_OK;
}

static bool watch_lsm6ds3_i2c_write(void *context, uint8_t address, uint8_t reg,
                                    const uint8_t *data, uint8_t length)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_Mem_Write(handle, (uint16_t)(address << 1U), reg, I2C_MEMADD_SIZE_8BIT,
                             (uint8_t *)data, length, WATCH_LSM6DS3_I2C_TIMEOUT_MS)
        == HAL_OK;
}

static bool watch_lsm6ds3_publish_sample(void *context, uint32_t timestamp_ms)
{
    watch_ui_event_t event;

    (void)context;
    event = (watch_ui_event_t) {
        .type = WATCH_LSM6DS3_SERVICE_EVENT_SAMPLE,
        .value = 0U,
        .timestamp_ms = timestamp_ms,
    };
    return watch_runtime_post_ui_event(&event);
}

static void watch_lsm6ds3_board_init_once(void)
{
    watch_lsm6ds3_bus_t bus;

    if (s_service_initialized) {
        return;
    }

    bus = (watch_lsm6ds3_bus_t) {
        .read = watch_lsm6ds3_i2c_read,
        .write = watch_lsm6ds3_i2c_write,
        .context = &hi2c1,
    };
    (void)watch_lsm6ds3_service_init(&s_service, &bus, watch_lsm6ds3_publish_sample, NULL);
    s_service_initialized = true;
}

void watch_lsm6ds3_board_process(uint32_t now_ms)
{
    watch_lsm6ds3_board_init_once();
    (void)watch_runtime_start_service(WATCH_RUNTIME_SERVICE_SENSOR, now_ms);
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_SENSOR, now_ms);
    (void)watch_lsm6ds3_service_process(&s_service, now_ms);
}

bool watch_lsm6ds3_board_read_status(watch_lsm6ds3_service_status_t *status)
{
    if (!s_service_initialized) {
        return false;
    }

    return watch_lsm6ds3_service_read_status(&s_service, status);
}

bool watch_lsm6ds3_board_read_latest(watch_lsm6ds3_sample_t *sample)
{
    if (!s_service_initialized) {
        return false;
    }

    return watch_lsm6ds3_service_read_latest(&s_service, sample);
}
