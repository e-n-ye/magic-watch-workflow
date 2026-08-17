#ifndef WATCH_MAX30102_BOARD_H
#define WATCH_MAX30102_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_max30102.h"

void watch_max30102_board_process(uint32_t now_ms);
bool watch_max30102_board_read_status(watch_max30102_service_status_t *status);
bool watch_max30102_board_read_latest(watch_max30102_sample_t *sample);

#endif /* WATCH_MAX30102_BOARD_H */
