#ifndef WATCH_EEPROM_PROBE_BOARD_H
#define WATCH_EEPROM_PROBE_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_eeprom_probe.h"

void watch_eeprom_probe_board_process(uint32_t now_ms);
bool watch_eeprom_probe_board_read_status(watch_eeprom_probe_status_t *status);

#endif /* WATCH_EEPROM_PROBE_BOARD_H */
