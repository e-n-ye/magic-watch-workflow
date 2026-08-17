#ifndef WATCH_LIS2MDL_H
#define WATCH_LIS2MDL_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_lsm6ds3_sensor_hub.h"

#define WATCH_LIS2MDL_I2C_ADDRESS 0x1EU
#define WATCH_LIS2MDL_WHO_AM_I_VALUE 0x40U
#define WATCH_LIS2MDL_SERVICE_PERIOD_MS 100U
#define WATCH_LIS2MDL_SERVICE_EVENT_SAMPLE 0x0101U

typedef enum {
    WATCH_LIS2MDL_RESULT_OK = 0,
    WATCH_LIS2MDL_RESULT_INVALID_ARGUMENT,
    WATCH_LIS2MDL_RESULT_BUS_ERROR,
    WATCH_LIS2MDL_RESULT_NACK,
    WATCH_LIS2MDL_RESULT_ID_MISMATCH,
    WATCH_LIS2MDL_RESULT_NOT_READY,
    WATCH_LIS2MDL_RESULT_COUNT
} watch_lis2mdl_result_t;

typedef enum {
    WATCH_LIS2MDL_SERVICE_STATE_NEW = 0,
    WATCH_LIS2MDL_SERVICE_STATE_ID_WAIT,
    WATCH_LIS2MDL_SERVICE_STATE_CONFIG_A_WAIT,
    WATCH_LIS2MDL_SERVICE_STATE_CONFIG_B_WAIT,
    WATCH_LIS2MDL_SERVICE_STATE_CONFIG_C_WAIT,
    WATCH_LIS2MDL_SERVICE_STATE_SAMPLE_WAIT,
    WATCH_LIS2MDL_SERVICE_STATE_READY,
    WATCH_LIS2MDL_SERVICE_STATE_FAILED,
    WATCH_LIS2MDL_SERVICE_STATE_COUNT
} watch_lis2mdl_service_state_t;

typedef struct
{
    int16_t magnetic_x;
    int16_t magnetic_y;
    int16_t magnetic_z;
} watch_lis2mdl_sample_t;

typedef bool (*watch_lis2mdl_publish_fn)(void *context, uint32_t timestamp_ms);

typedef struct
{
    watch_lsm6ds3_sensor_hub_t hub;
    watch_lis2mdl_publish_fn publish;
    void *publish_context;
    watch_lis2mdl_sample_t latest_sample;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t nack_count;
    uint32_t event_drop_count;
    uint8_t who_am_i;
    watch_lis2mdl_service_state_t state;
    bool initialized;
    bool ready;
    bool sample_valid;
} watch_lis2mdl_service_t;

typedef struct
{
    bool ready;
    uint8_t who_am_i;
    bool sample_valid;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t nack_count;
    uint32_t event_drop_count;
    watch_lis2mdl_service_state_t state;
} watch_lis2mdl_service_status_t;

bool watch_lis2mdl_validate_identity(uint8_t who_am_i);
bool watch_lis2mdl_decode_sample(const uint8_t data[6], watch_lis2mdl_sample_t *sample);
bool watch_lis2mdl_service_init(watch_lis2mdl_service_t *service, const watch_lsm6ds3_bus_t *bus,
                                watch_lis2mdl_publish_fn publish, void *publish_context);
bool watch_lis2mdl_service_process(watch_lis2mdl_service_t *service, uint32_t now_ms);
bool watch_lis2mdl_service_read_status(const watch_lis2mdl_service_t *service,
                                       watch_lis2mdl_service_status_t *status);
bool watch_lis2mdl_service_read_latest(const watch_lis2mdl_service_t *service,
                                       watch_lis2mdl_sample_t *sample);

#endif /* WATCH_LIS2MDL_H */
