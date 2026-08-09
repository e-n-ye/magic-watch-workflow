#ifndef WATCH_WATCHDOG_H
#define WATCH_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_WATCHDOG_REFRESH_PERIOD_MS 100U

typedef bool (*watch_watchdog_refresh_fn)(void *context);

typedef struct
{
    bool initialized;
    bool enabled;
    watch_watchdog_refresh_fn refresh;
    void *context;
    uint32_t last_refresh_ms;
    uint32_t refresh_count;
    uint32_t blocked_count;
    uint32_t refresh_failure_count;
} watch_watchdog_t;

typedef struct
{
    bool enabled;
    uint32_t last_refresh_ms;
    uint32_t refresh_count;
    uint32_t blocked_count;
    uint32_t refresh_failure_count;
} watch_watchdog_status_t;

bool watch_watchdog_init(watch_watchdog_t *watchdog, watch_watchdog_refresh_fn refresh,
                         void *context, uint32_t now_ms);
bool watch_watchdog_process(watch_watchdog_t *watchdog, uint32_t now_ms, bool system_healthy);
bool watch_watchdog_read_status(const watch_watchdog_t *watchdog, watch_watchdog_status_t *status);

#endif /* WATCH_WATCHDOG_H */
