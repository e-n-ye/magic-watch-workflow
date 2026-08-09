#include "watch_power_state.h"

#include <stddef.h>

static watch_power_wake_source_t watch_power_state_wake_source(watch_power_event_t event)
{
    return event == WATCH_POWER_EVENT_WAKE_RTC ? WATCH_POWER_WAKE_RTC : WATCH_POWER_WAKE_KEY;
}

bool watch_power_state_init(watch_power_state_t *power)
{
    if (power == NULL) {
        return false;
    }

    *power = (watch_power_state_t) {
        .snapshot = {
            .state = WATCH_POWER_STATE_ACTIVE,
            .wake_source = WATCH_POWER_WAKE_NONE,
            .transition_count = 0U,
        },
        .initialized = true,
    };
    return true;
}

bool watch_power_state_dispatch(watch_power_state_t *power, watch_power_event_t event)
{
    watch_power_state_id_t next_state;
    watch_power_wake_source_t next_wake_source;

    if (power == NULL || !power->initialized || event >= WATCH_POWER_EVENT_COUNT) {
        return false;
    }

    next_state = power->snapshot.state;
    next_wake_source = power->snapshot.wake_source;

    switch (event) {
    case WATCH_POWER_EVENT_DISPLAY_TIMEOUT:
        if (power->snapshot.state == WATCH_POWER_STATE_ACTIVE) {
            next_state = WATCH_POWER_STATE_DISPLAY_OFF;
            next_wake_source = WATCH_POWER_WAKE_NONE;
        }
        break;
    case WATCH_POWER_EVENT_STOP_REQUEST:
        if (power->snapshot.state == WATCH_POWER_STATE_ACTIVE
            || power->snapshot.state == WATCH_POWER_STATE_DISPLAY_OFF) {
            next_state = WATCH_POWER_STATE_STOPPED;
            next_wake_source = WATCH_POWER_WAKE_NONE;
        }
        break;
    case WATCH_POWER_EVENT_WAKE_KEY:
    case WATCH_POWER_EVENT_WAKE_RTC:
        if (power->snapshot.state != WATCH_POWER_STATE_ACTIVE) {
            next_state = WATCH_POWER_STATE_ACTIVE;
            next_wake_source = watch_power_state_wake_source(event);
        }
        break;
    case WATCH_POWER_EVENT_SOFTWARE_OFF:
        if (power->snapshot.state != WATCH_POWER_STATE_OFF) {
            next_state = WATCH_POWER_STATE_OFF;
            next_wake_source = WATCH_POWER_WAKE_NONE;
        }
        break;
    case WATCH_POWER_EVENT_COUNT:
        return false;
    }

    if (next_state == power->snapshot.state && next_wake_source == power->snapshot.wake_source) {
        return false;
    }

    power->snapshot.state = next_state;
    power->snapshot.wake_source = next_wake_source;
    power->snapshot.transition_count++;
    return true;
}

bool watch_power_state_read(const watch_power_state_t *power, watch_power_snapshot_t *snapshot)
{
    if (power == NULL || snapshot == NULL || !power->initialized) {
        return false;
    }

    *snapshot = power->snapshot;
    return true;
}
