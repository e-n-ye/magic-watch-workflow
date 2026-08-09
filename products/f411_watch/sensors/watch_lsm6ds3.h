#ifndef WATCH_LSM6DS3_H
#define WATCH_LSM6DS3_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_LSM6DS3_I2C_ADDRESS 0x6AU
#define WATCH_LSM6DS3_WHO_AM_I_VALUE 0x6AU
#define WATCH_LSM6DS3_SERVICE_PERIOD_MS 20U
#define WATCH_LSM6DS3_SERVICE_EVENT_SAMPLE 0x0100U

typedef enum {
    WATCH_LSM6DS3_RESULT_OK = 0,
    WATCH_LSM6DS3_RESULT_INVALID_ARGUMENT,
    WATCH_LSM6DS3_RESULT_BUS_ERROR,
    WATCH_LSM6DS3_RESULT_ID_MISMATCH,
    WATCH_LSM6DS3_RESULT_NOT_READY,
    WATCH_LSM6DS3_RESULT_COUNT
} watch_lsm6ds3_result_t;

typedef bool (*watch_lsm6ds3_read_fn)(void *context, uint8_t address, uint8_t reg, uint8_t *data,
                                      uint8_t length);
typedef bool (*watch_lsm6ds3_write_fn)(void *context, uint8_t address, uint8_t reg,
                                       const uint8_t *data, uint8_t length);

typedef struct
{
    watch_lsm6ds3_read_fn read;
    watch_lsm6ds3_write_fn write;
    void *context;
} watch_lsm6ds3_bus_t;

typedef struct
{
    watch_lsm6ds3_bus_t bus;
    uint8_t who_am_i;
    bool initialized;
} watch_lsm6ds3_t;

typedef struct
{
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
} watch_lsm6ds3_sample_t;

typedef bool (*watch_lsm6ds3_publish_fn)(void *context, uint32_t timestamp_ms);

typedef struct
{
    watch_lsm6ds3_t device;
    watch_lsm6ds3_publish_fn publish;
    void *publish_context;
    watch_lsm6ds3_sample_t latest_sample;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t event_drop_count;
    bool initialized;
    bool ready;
    bool sample_valid;
} watch_lsm6ds3_service_t;

typedef struct
{
    bool ready;
    uint8_t who_am_i;
    bool sample_valid;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t event_drop_count;
} watch_lsm6ds3_service_status_t;

watch_lsm6ds3_result_t watch_lsm6ds3_init(watch_lsm6ds3_t *device, const watch_lsm6ds3_bus_t *bus);
watch_lsm6ds3_result_t watch_lsm6ds3_read_sample(const watch_lsm6ds3_t *device,
                                                 watch_lsm6ds3_sample_t *sample);

bool watch_lsm6ds3_service_init(watch_lsm6ds3_service_t *service, const watch_lsm6ds3_bus_t *bus,
                                watch_lsm6ds3_publish_fn publish, void *publish_context);
bool watch_lsm6ds3_service_process(watch_lsm6ds3_service_t *service, uint32_t now_ms);
bool watch_lsm6ds3_service_read_status(const watch_lsm6ds3_service_t *service,
                                       watch_lsm6ds3_service_status_t *status);
bool watch_lsm6ds3_service_read_latest(const watch_lsm6ds3_service_t *service,
                                       watch_lsm6ds3_sample_t *sample);

#endif /* WATCH_LSM6DS3_H */
