#ifndef WATCH_LSM6DS3_BOARD_H
#define WATCH_LSM6DS3_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_lsm6ds3.h"

void watch_lsm6ds3_board_process(uint32_t now_ms);
bool watch_lsm6ds3_board_read_status(watch_lsm6ds3_service_status_t *status);
bool watch_lsm6ds3_board_read_latest(watch_lsm6ds3_sample_t *sample);

#endif /* WATCH_LSM6DS3_BOARD_H */
