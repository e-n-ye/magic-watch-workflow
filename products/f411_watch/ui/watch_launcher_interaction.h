/**
 * @file watch_launcher_interaction.h
 * @brief XML-aligned launcher touch routing.
 */

#ifndef WATCH_LAUNCHER_INTERACTION_H
#define WATCH_LAUNCHER_INTERACTION_H

#include <stdbool.h>
#include <stdint.h>

#include "../core/watch_core.h"

bool watch_launcher_hit_test(uint16_t x, uint16_t y, uint8_t *launcher_index);
bool watch_launcher_map_touch(const watch_snapshot_t *snapshot, const watch_event_t *touch_event,
                              watch_event_t *mapped_event);

#endif /* WATCH_LAUNCHER_INTERACTION_H */
