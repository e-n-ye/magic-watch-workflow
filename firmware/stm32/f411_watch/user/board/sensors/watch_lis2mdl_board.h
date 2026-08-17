#ifndef WATCH_LIS2MDL_BOARD_H
#define WATCH_LIS2MDL_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_lis2mdl.h"

void watch_lis2mdl_board_process(uint32_t now_ms);
bool watch_lis2mdl_board_read_status(watch_lis2mdl_service_status_t *status);
bool watch_lis2mdl_board_read_latest(watch_lis2mdl_sample_t *sample);

#endif /* WATCH_LIS2MDL_BOARD_H */
