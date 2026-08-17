#ifndef WATCH_AHT20_BOARD_H
#define WATCH_AHT20_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_aht20.h"

void watch_aht20_board_process(uint32_t now_ms);
bool watch_aht20_board_read_status(watch_aht20_service_status_t *status);
bool watch_aht20_board_read_latest(watch_aht20_sample_t *sample);

#endif /* WATCH_AHT20_BOARD_H */
