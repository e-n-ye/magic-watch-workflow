#include "board/sensors/watch_max30102_board.h"

#include "i2c.h"
#include "watch_runtime.h"

#define WATCH_MAX30102_I2C_TIMEOUT_MS 100U

static watch_max30102_service_t s_service;
static bool s_service_initialized;

static bool watch_max30102_i2c_read(void *context, uint8_t address, uint8_t reg, uint8_t *data,
                                    uint8_t length)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_Mem_Read(handle, (uint16_t)(address << 1U), reg, I2C_MEMADD_SIZE_8BIT, data,
                            length, WATCH_MAX30102_I2C_TIMEOUT_MS)
        == HAL_OK;
}

static bool watch_max30102_i2c_write(void *context, uint8_t address, uint8_t reg,
                                     const uint8_t *data, uint8_t length)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_Mem_Write(handle, (uint16_t)(address << 1U), reg, I2C_MEMADD_SIZE_8BIT,
                             (uint8_t *)data, length, WATCH_MAX30102_I2C_TIMEOUT_MS)
        == HAL_OK;
}

static bool watch_max30102_publish_sample(void *context, uint32_t timestamp_ms)
{
    const watch_ui_event_t event = {
        .type = WATCH_MAX30102_SERVICE_EVENT_SAMPLE,
        .value = 0U,
        .timestamp_ms = timestamp_ms,
    };

    (void)context;
    return watch_runtime_post_ui_event(&event);
}

static void watch_max30102_board_init_once(void)
{
    const watch_max30102_bus_t bus = {
        .read = watch_max30102_i2c_read,
        .write = watch_max30102_i2c_write,
        .context = &hi2c1,
    };

    if (s_service_initialized) {
        return;
    }

    s_service_initialized =
        watch_max30102_service_init(&s_service, &bus, watch_max30102_publish_sample, NULL);
}

void watch_max30102_board_process(uint32_t now_ms)
{
    watch_max30102_board_init_once();
    (void)watch_runtime_start_service(WATCH_RUNTIME_SERVICE_SENSOR, now_ms);
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_SENSOR, now_ms);
    (void)watch_max30102_service_process(&s_service, now_ms);
}

bool watch_max30102_board_read_status(watch_max30102_service_status_t *status)
{
    return s_service_initialized && watch_max30102_service_read_status(&s_service, status);
}

bool watch_max30102_board_read_latest(watch_max30102_sample_t *sample)
{
    return s_service_initialized && watch_max30102_service_read_latest(&s_service, sample);
}
