/**
 * @file watch_launcher_interaction.c
 * @brief Keep launcher hit targets aligned with the exported 240x280 XML geometry.
 */

#include "watch_launcher_interaction.h"

#include <stddef.h>

#define WATCH_LAUNCHER_CARD_X 16U
#define WATCH_LAUNCHER_CARD_WIDTH 208U
#define WATCH_LAUNCHER_CARD_HEIGHT 40U

typedef struct
{
    uint16_t y;
    uint8_t index;
} watch_launcher_card_t;

static const watch_launcher_card_t s_launcher_cards[WATCH_CORE_LAUNCHER_ITEM_COUNT] = {
    { 58U, 0U },
    { 102U, 1U },
    { 146U, 2U },
    { 190U, 3U },
};

bool watch_launcher_hit_test(uint16_t x, uint16_t y, uint8_t *launcher_index)
{
    uint8_t card_index;

    if (launcher_index == NULL || x < WATCH_LAUNCHER_CARD_X
        || x >= WATCH_LAUNCHER_CARD_X + WATCH_LAUNCHER_CARD_WIDTH) {
        return false;
    }

    for (card_index = 0U; card_index < WATCH_CORE_LAUNCHER_ITEM_COUNT; ++card_index) {
        const watch_launcher_card_t *card = &s_launcher_cards[card_index];

        if (y >= card->y && y < card->y + WATCH_LAUNCHER_CARD_HEIGHT) {
            *launcher_index = card->index;
            return true;
        }
    }

    return false;
}

bool watch_launcher_map_touch(const watch_snapshot_t *snapshot, const watch_event_t *touch_event,
                              watch_event_t *mapped_event)
{
    uint8_t launcher_index;

    if (snapshot == NULL || touch_event == NULL || mapped_event == NULL
        || snapshot->page != WATCH_PAGE_LAUNCHER || touch_event->type != WATCH_EVENT_SELECT
        || !touch_event->touch_valid
        || !watch_launcher_hit_test(touch_event->touch_x, touch_event->touch_y, &launcher_index)) {
        return false;
    }

    *mapped_event = *touch_event;
    mapped_event->type = WATCH_EVENT_LAUNCHER_ITEM_TAPPED;
    mapped_event->launcher_index = launcher_index;
    return true;
}
