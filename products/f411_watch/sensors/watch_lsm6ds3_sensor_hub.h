#ifndef WATCH_LSM6DS3_SENSOR_HUB_H
#define WATCH_LSM6DS3_SENSOR_HUB_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_lsm6ds3.h"

#define WATCH_LSM6DS3_SENSOR_HUB_CYCLE_WAIT_MS 20U
#define WATCH_LSM6DS3_SENSOR_HUB_CYCLE_TIMEOUT_MS 100U

typedef enum {
    WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK = 0,
    WATCH_LSM6DS3_SENSOR_HUB_RESULT_INVALID_ARGUMENT,
    WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR,
    WATCH_LSM6DS3_SENSOR_HUB_RESULT_NACK,
    WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY,
    WATCH_LSM6DS3_SENSOR_HUB_RESULT_COUNT
} watch_lsm6ds3_sensor_hub_result_t;

typedef struct
{
    watch_lsm6ds3_bus_t bus;
    uint32_t operation_started_ms;
    bool initialized;
    bool operation_pending;
} watch_lsm6ds3_sensor_hub_t;

bool watch_lsm6ds3_sensor_hub_init(watch_lsm6ds3_sensor_hub_t *hub, const watch_lsm6ds3_bus_t *bus);
watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_configure_read(watch_lsm6ds3_sensor_hub_t *hub, uint8_t slave_address,
                                        uint8_t reg, uint8_t length, uint32_t now_ms);
watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_configure_write(watch_lsm6ds3_sensor_hub_t *hub, uint8_t slave_address,
                                         uint8_t reg, uint8_t value, uint32_t now_ms);
watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_start_read(watch_lsm6ds3_sensor_hub_t *hub, uint32_t now_ms);
watch_lsm6ds3_sensor_hub_result_t watch_lsm6ds3_sensor_hub_wait(watch_lsm6ds3_sensor_hub_t *hub,
                                                                uint32_t now_ms);
watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_check_status(const watch_lsm6ds3_sensor_hub_t *hub);
watch_lsm6ds3_sensor_hub_result_t
watch_lsm6ds3_sensor_hub_read_cache(const watch_lsm6ds3_sensor_hub_t *hub, uint8_t *data,
                                    uint8_t length);

#endif /* WATCH_LSM6DS3_SENSOR_HUB_H */
