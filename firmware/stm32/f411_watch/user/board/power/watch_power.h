#ifndef WATCH_POWER_H
#define WATCH_POWER_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_power_state.h"
#include "watch_watchdog.h"

typedef struct
{
    watch_power_snapshot_t power;
    bool watchdog_enabled;
    uint32_t watchdog_refresh_count;
    uint32_t watchdog_blocked_count;
    uint32_t watchdog_refresh_failure_count;
    uint32_t stop_count;
    uint32_t wake_count;
} watch_power_board_status_t;

void watch_power_latch_early(void);
bool watch_power_board_init(uint32_t now_ms);
void watch_power_board_process(uint32_t now_ms);
bool watch_power_board_request_display_off(void);
bool watch_power_board_request_stop(void);
bool watch_power_board_request_software_off(void);
void watch_power_board_note_wake_key(void);
void watch_power_board_notify_wake(watch_power_wake_source_t source);
bool watch_power_board_read_status(watch_power_board_status_t *status);

#endif /* WATCH_POWER_H */
