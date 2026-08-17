#ifndef WATCH_SENSOR_AGGREGATE_H
#define WATCH_SENSOR_AGGREGATE_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_SENSOR_AGGREGATE_SENSOR_COUNT 5U
#define WATCH_SENSOR_AGGREGATE_SERVICE_EVENT_STATUS 0x0110U
#define WATCH_SENSOR_AGGREGATE_ALL_AVAILABLE_MASK                                                  \
    ((uint8_t)((1U << WATCH_SENSOR_AGGREGATE_SENSOR_COUNT) - 1U))
#define WATCH_SENSOR_AGGREGATE_STATE_UNAVAILABLE 0xFFU

typedef enum {
    WATCH_SENSOR_AGGREGATE_LSM6DS3 = 0,
    WATCH_SENSOR_AGGREGATE_LIS2MDL,
    WATCH_SENSOR_AGGREGATE_AHT20,
    WATCH_SENSOR_AGGREGATE_CW2015,
    WATCH_SENSOR_AGGREGATE_MAX30102,
    WATCH_SENSOR_AGGREGATE_SENSOR_ID_COUNT
} watch_sensor_aggregate_sensor_id_t;

typedef struct
{
    bool ready;
    bool sample_valid;
    uint8_t state;
    uint32_t last_sample_ms;
    uint32_t sample_count;
    uint32_t error_count;
} watch_sensor_aggregate_status_t;

typedef struct
{
    watch_sensor_aggregate_status_t sensors[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT];
    uint8_t available_mask;
    bool degraded;
    uint32_t revision;
} watch_sensor_aggregate_snapshot_t;

typedef struct
{
    watch_sensor_aggregate_snapshot_t snapshot;
    bool initialized;
} watch_sensor_aggregate_service_t;

bool watch_sensor_aggregate_init(watch_sensor_aggregate_service_t *service);
bool watch_sensor_aggregate_update(
    watch_sensor_aggregate_service_t *service,
    const watch_sensor_aggregate_status_t statuses[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT],
    watch_sensor_aggregate_snapshot_t *snapshot);
bool watch_sensor_aggregate_read_snapshot(const watch_sensor_aggregate_service_t *service,
                                          watch_sensor_aggregate_snapshot_t *snapshot);
bool watch_sensor_aggregate_snapshot_is_valid(const watch_sensor_aggregate_snapshot_t *snapshot);
bool watch_sensor_aggregate_snapshot_equal(const watch_sensor_aggregate_snapshot_t *left,
                                           const watch_sensor_aggregate_snapshot_t *right);

#endif /* WATCH_SENSOR_AGGREGATE_H */
