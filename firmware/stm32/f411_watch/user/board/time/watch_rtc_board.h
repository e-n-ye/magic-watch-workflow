#ifndef WATCH_RTC_BOARD_H
#define WATCH_RTC_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_time.h"

void watch_rtc_board_capture_boot(void);
void watch_rtc_board_restore_boot(void);
bool watch_rtc_board_init(uint32_t now_ms, watch_time_value_t *time);
bool watch_rtc_board_process(uint32_t now_ms, watch_time_value_t *time);

#endif /* WATCH_RTC_BOARD_H */
