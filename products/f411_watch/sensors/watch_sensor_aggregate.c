#include "watch_sensor_aggregate.h"

#include <stddef.h>

static bool watch_sensor_aggregate_status_equal(const watch_sensor_aggregate_status_t *left,
                                                const watch_sensor_aggregate_status_t *right)
{
    return left->ready == right->ready && left->sample_valid == right->sample_valid
        && left->state == right->state && left->last_sample_ms == right->last_sample_ms
        && left->sample_count == right->sample_count && left->error_count == right->error_count;
}

bool watch_sensor_aggregate_snapshot_is_valid(const watch_sensor_aggregate_snapshot_t *snapshot)
{
    if (snapshot == NULL
        || (snapshot->available_mask & (uint8_t)~WATCH_SENSOR_AGGREGATE_ALL_AVAILABLE_MASK) != 0U
        || snapshot->degraded
            != (snapshot->available_mask != WATCH_SENSOR_AGGREGATE_ALL_AVAILABLE_MASK)) {
        return false;
    }

    for (uint8_t index = 0U; index < WATCH_SENSOR_AGGREGATE_SENSOR_COUNT; index++) {
        bool available = (snapshot->available_mask & (uint8_t)(1U << index)) != 0U;

        if (snapshot->sensors[index].ready != available) {
            return false;
        }
    }

    return true;
}

bool watch_sensor_aggregate_snapshot_equal(const watch_sensor_aggregate_snapshot_t *left,
                                           const watch_sensor_aggregate_snapshot_t *right)
{
    if (left == NULL || right == NULL || left->available_mask != right->available_mask
        || left->degraded != right->degraded || left->revision != right->revision) {
        return false;
    }

    for (uint8_t index = 0U; index < WATCH_SENSOR_AGGREGATE_SENSOR_COUNT; index++) {
        if (!watch_sensor_aggregate_status_equal(&left->sensors[index], &right->sensors[index])) {
            return false;
        }
    }

    return true;
}

bool watch_sensor_aggregate_init(watch_sensor_aggregate_service_t *service)
{
    if (service == NULL) {
        return false;
    }

    *service = (watch_sensor_aggregate_service_t) { 0 };
    service->initialized = true;
    return true;
}

bool watch_sensor_aggregate_update(
    watch_sensor_aggregate_service_t *service,
    const watch_sensor_aggregate_status_t statuses[WATCH_SENSOR_AGGREGATE_SENSOR_COUNT],
    watch_sensor_aggregate_snapshot_t *snapshot)
{
    watch_sensor_aggregate_snapshot_t next = { 0 };

    if (service == NULL || statuses == NULL || !service->initialized) {
        return false;
    }

    for (uint8_t index = 0U; index < WATCH_SENSOR_AGGREGATE_SENSOR_COUNT; index++) {
        next.sensors[index] = statuses[index];
        if (statuses[index].ready) {
            next.available_mask |= (uint8_t)(1U << index);
        }
    }
    next.degraded = next.available_mask != WATCH_SENSOR_AGGREGATE_ALL_AVAILABLE_MASK;
    next.revision = service->snapshot.revision;

    if (!watch_sensor_aggregate_snapshot_is_valid(&next)) {
        return false;
    }

    if (!service->snapshot.revision
        || !watch_sensor_aggregate_snapshot_equal(&service->snapshot, &next)) {
        next.revision++;
        service->snapshot = next;
        if (snapshot != NULL) {
            *snapshot = service->snapshot;
        }
        return true;
    }

    if (snapshot != NULL) {
        *snapshot = service->snapshot;
    }
    return false;
}

bool watch_sensor_aggregate_read_snapshot(const watch_sensor_aggregate_service_t *service,
                                          watch_sensor_aggregate_snapshot_t *snapshot)
{
    if (service == NULL || snapshot == NULL || !service->initialized) {
        return false;
    }

    *snapshot = service->snapshot;
    return true;
}
