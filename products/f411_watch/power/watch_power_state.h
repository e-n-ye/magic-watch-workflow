#ifndef WATCH_POWER_STATE_H
#define WATCH_POWER_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WATCH_POWER_STATE_ACTIVE = 0,
    WATCH_POWER_STATE_DISPLAY_OFF,
    WATCH_POWER_STATE_STOPPED,
    WATCH_POWER_STATE_OFF,
    WATCH_POWER_STATE_COUNT
} watch_power_state_id_t;

typedef enum {
    WATCH_POWER_WAKE_NONE = 0,
    WATCH_POWER_WAKE_KEY,
    WATCH_POWER_WAKE_RTC,
    WATCH_POWER_WAKE_RESET,
    WATCH_POWER_WAKE_COUNT
} watch_power_wake_source_t;

typedef enum {
    WATCH_POWER_EVENT_DISPLAY_TIMEOUT = 0,
    WATCH_POWER_EVENT_STOP_REQUEST,
    WATCH_POWER_EVENT_WAKE_KEY,
    WATCH_POWER_EVENT_WAKE_RTC,
    WATCH_POWER_EVENT_SOFTWARE_OFF,
    WATCH_POWER_EVENT_COUNT
} watch_power_event_t;

typedef struct
{
    watch_power_state_id_t state;
    watch_power_wake_source_t wake_source;
    uint32_t transition_count;
} watch_power_snapshot_t;

typedef struct
{
    watch_power_snapshot_t snapshot;
    bool initialized;
} watch_power_state_t;

bool watch_power_state_init(watch_power_state_t *power);
bool watch_power_state_dispatch(watch_power_state_t *power, watch_power_event_t event);
bool watch_power_state_read(const watch_power_state_t *power, watch_power_snapshot_t *snapshot);

#endif /* WATCH_POWER_STATE_H */
