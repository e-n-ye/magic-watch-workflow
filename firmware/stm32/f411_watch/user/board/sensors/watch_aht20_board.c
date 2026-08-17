#include "board/sensors/watch_aht20_board.h"

#include "i2c.h"
#include "watch_runtime.h"

#define WATCH_AHT20_I2C_TIMEOUT_MS 100U

static watch_aht20_service_t s_service;
static bool s_service_initialized;

static bool watch_aht20_i2c_write(void *context, uint8_t address, const uint8_t *data,
                                  uint8_t length)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_Master_Transmit(handle, (uint16_t)(address << 1U), (uint8_t *)data, length,
                                   WATCH_AHT20_I2C_TIMEOUT_MS)
        == HAL_OK;
}

static bool watch_aht20_i2c_read(void *context, uint8_t address, uint8_t *data, uint8_t length)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_Master_Receive(handle, (uint16_t)(address << 1U), data, length,
                                  WATCH_AHT20_I2C_TIMEOUT_MS)
        == HAL_OK;
}

static bool watch_aht20_publish_sample(void *context, uint32_t timestamp_ms)
{
    const watch_ui_event_t event = {
        .type = WATCH_AHT20_SERVICE_EVENT_SAMPLE,
        .value = 0U,
        .timestamp_ms = timestamp_ms,
    };

    (void)context;
    return watch_runtime_post_ui_event(&event);
}

static void watch_aht20_board_init_once(void)
{
    const watch_aht20_bus_t bus = {
        .write = watch_aht20_i2c_write,
        .read = watch_aht20_i2c_read,
        .context = &hi2c1,
    };

    if (s_service_initialized) {
        return;
    }

    s_service_initialized =
        watch_aht20_service_init(&s_service, &bus, watch_aht20_publish_sample, NULL);
}

void watch_aht20_board_process(uint32_t now_ms)
{
    watch_aht20_board_init_once();
    (void)watch_runtime_start_service(WATCH_RUNTIME_SERVICE_SENSOR, now_ms);
    (void)watch_runtime_heartbeat(WATCH_RUNTIME_SERVICE_SENSOR, now_ms);
    (void)watch_aht20_service_process(&s_service, now_ms);
}

bool watch_aht20_board_read_status(watch_aht20_service_status_t *status)
{
    return s_service_initialized && watch_aht20_service_read_status(&s_service, status);
}

bool watch_aht20_board_read_latest(watch_aht20_sample_t *sample)
{
    return s_service_initialized && watch_aht20_service_read_latest(&s_service, sample);
}
