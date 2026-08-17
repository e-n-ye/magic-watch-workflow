#ifndef WATCH_CW2015_BOARD_H
#define WATCH_CW2015_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_cw2015.h"

void watch_cw2015_board_process(uint32_t now_ms);
bool watch_cw2015_board_read_status(watch_cw2015_service_status_t *status);
bool watch_cw2015_board_read_latest(watch_cw2015_sample_t *sample);

#endif /* WATCH_CW2015_BOARD_H */
