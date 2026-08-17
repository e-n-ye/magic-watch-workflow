#ifndef WATCH_APP_H
#define WATCH_APP_H

#include <stdbool.h>

#include "watch_core.h"

void watch_app_init(void);
bool watch_app_is_ready(void);
void watch_app_process(void);
bool watch_app_read_snapshot(watch_snapshot_t *snapshot);
bool watch_app_dispatch_sensor_snapshot(const watch_sensor_aggregate_snapshot_t *sensor_snapshot);

#endif /* WATCH_APP_H */
