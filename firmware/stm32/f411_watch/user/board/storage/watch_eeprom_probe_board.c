#include "board/storage/watch_eeprom_probe_board.h"

#include "i2c.h"

#define WATCH_EEPROM_PROBE_TIMEOUT_MS 10U

static watch_eeprom_probe_service_t s_service;
static bool s_service_initialized;

static bool watch_eeprom_probe_is_ready(void *context, uint8_t address)
{
    I2C_HandleTypeDef *handle = (I2C_HandleTypeDef *)context;

    return handle != NULL
        && HAL_I2C_IsDeviceReady(handle, (uint16_t)(address << 1U), 1U,
                                 WATCH_EEPROM_PROBE_TIMEOUT_MS)
        == HAL_OK;
}

static void watch_eeprom_probe_board_init_once(void)
{
    if (s_service_initialized) {
        return;
    }

    s_service_initialized =
        watch_eeprom_probe_init(&s_service, watch_eeprom_probe_is_ready, &hi2c1);
}

void watch_eeprom_probe_board_process(uint32_t now_ms)
{
    (void)now_ms;
    watch_eeprom_probe_board_init_once();
    (void)watch_eeprom_probe_process(&s_service);
}

bool watch_eeprom_probe_board_read_status(watch_eeprom_probe_status_t *status)
{
    return s_service_initialized && watch_eeprom_probe_read_status(&s_service, status);
}
