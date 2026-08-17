#ifndef WATCH_SENSOR_AGGREGATE_BOARD_H
#define WATCH_SENSOR_AGGREGATE_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_sensor_aggregate.h"

void watch_sensor_aggregate_board_process(uint32_t now_ms);
bool watch_sensor_aggregate_board_read_snapshot(watch_sensor_aggregate_snapshot_t *snapshot);

#endif /* WATCH_SENSOR_AGGREGATE_BOARD_H */
