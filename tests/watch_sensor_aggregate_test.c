#include "watch_sensor_aggregate.h"

#include <assert.h>
#include <stdint.h>

static watch_sensor_aggregate_status_t ready_status(uint32_t sample_count)
{
    return (watch_sensor_aggregate_status_t) {
        .ready = true,
        .sample_valid = true,
        .state = 1U,
        .last_sample_ms = sample_count * 10U,
        .sample_count = sample_count,
        .error_count = 0U,
    };
}

static void test_degraded_snapshot_and_coalescing(void)
{
    watch_sensor_aggregate_service_t service;
    watch_sensor_aggregate_snapshot_t snapshot;
    watch_sensor_aggregate_status_t statuses[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT] = { 0 };

    assert(watch_sensor_aggregate_init(&service));
    assert(watch_sensor_aggregate_update(&service, statuses, &snapshot));
    assert(snapshot.revision == 1U);
    assert(snapshot.available_mask == 0U);
    assert(snapshot.degraded);
    assert(watch_sensor_aggregate_snapshot_is_valid(&snapshot));
    assert(!watch_sensor_aggregate_update(&service, statuses, &snapshot));
    assert(snapshot.revision == 1U);
}

static void test_all_sensors_and_change_revision(void)
{
    watch_sensor_aggregate_service_t service;
    watch_sensor_aggregate_snapshot_t snapshot;
    watch_sensor_aggregate_status_t statuses[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT];

    for (uint8_t index = 0U; index < WATCH_SENSOR_AGGREGATE_SENSOR_COUNT; index++) {
        statuses[index] = ready_status(index + 1U);
    }

    assert(watch_sensor_aggregate_init(&service));
    assert(watch_sensor_aggregate_update(&service, statuses, &snapshot));
    assert(snapshot.available_mask == WATCH_SENSOR_AGGREGATE_ALL_AVAILABLE_MASK);
    assert(!snapshot.degraded);
    assert(snapshot.revision == 1U);

    statuses[WATCH_SENSOR_AGGREGATE_MAX30102].sample_count++;
    assert(watch_sensor_aggregate_update(&service, statuses, &snapshot));
    assert(snapshot.revision == 2U);
    assert(snapshot.sensors[WATCH_SENSOR_AGGREGATE_MAX30102].sample_count == 6U);

    assert(watch_sensor_aggregate_read_snapshot(&service, &snapshot));
    assert(watch_sensor_aggregate_snapshot_is_valid(&snapshot));
}

static void test_snapshot_validation(void)
{
    watch_sensor_aggregate_snapshot_t snapshot = { 0 };

    snapshot.sensors[WATCH_SENSOR_AGGREGATE_LSM6DS3].ready = true;
    assert(!watch_sensor_aggregate_snapshot_is_valid(&snapshot));
    snapshot.available_mask = 1U;
    snapshot.degraded = true;
    assert(watch_sensor_aggregate_snapshot_is_valid(&snapshot));
    snapshot.available_mask = 0x80U;
    assert(!watch_sensor_aggregate_snapshot_is_valid(&snapshot));
}

int main(void)
{
    test_degraded_snapshot_and_coalescing();
    test_all_sensors_and_change_revision();
    test_snapshot_validation();
    return 0;
}
