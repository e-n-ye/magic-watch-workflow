#include "watch_watchdog.h"

#include <stddef.h>

bool watch_watchdog_init(watch_watchdog_t *watchdog, watch_watchdog_refresh_fn refresh,
                         void *context, uint32_t now_ms)
{
    if (watchdog == NULL || refresh == NULL) {
        return false;
    }

    *watchdog = (watch_watchdog_t) {
        .initialized = true,
        .enabled = true,
        .refresh = refresh,
        .context = context,
        .last_refresh_ms = now_ms - WATCH_WATCHDOG_REFRESH_PERIOD_MS,
    };
    return true;
}

bool watch_watchdog_process(watch_watchdog_t *watchdog, uint32_t now_ms, bool system_healthy)
{
    if (watchdog == NULL || !watchdog->initialized || !watchdog->enabled) {
        return false;
    }
    if (!system_healthy) {
        watchdog->blocked_count++;
        return false;
    }
    if ((now_ms - watchdog->last_refresh_ms) < WATCH_WATCHDOG_REFRESH_PERIOD_MS) {
        return true;
    }
    if (!watchdog->refresh(watchdog->context)) {
        watchdog->refresh_failure_count++;
        return false;
    }

    watchdog->last_refresh_ms = now_ms;
    watchdog->refresh_count++;
    return true;
}

bool watch_watchdog_read_status(const watch_watchdog_t *watchdog, watch_watchdog_status_t *status)
{
    if (watchdog == NULL || status == NULL || !watchdog->initialized) {
        return false;
    }

    status->enabled = watchdog->enabled;
    status->last_refresh_ms = watchdog->last_refresh_ms;
    status->refresh_count = watchdog->refresh_count;
    status->blocked_count = watchdog->blocked_count;
    status->refresh_failure_count = watchdog->refresh_failure_count;
    return true;
}
