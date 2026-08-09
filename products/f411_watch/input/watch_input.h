#ifndef WATCH_INPUT_H
#define WATCH_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_core.h"

#define WATCH_INPUT_EVENT_CAPACITY 8U
#define WATCH_INPUT_BUTTON_DEBOUNCE_MS 30U
#define WATCH_INPUT_TOUCH_EDGE_PX 24U
#define WATCH_INPUT_TOUCH_SWIPE_MIN_PX 40U

typedef enum {
    WATCH_INPUT_BUTTON_BACK = 0,
    WATCH_INPUT_BUTTON_WAKE,
    WATCH_INPUT_BUTTON_ENCODER,
    WATCH_INPUT_BUTTON_COUNT
} watch_input_button_t;

typedef struct
{
    uint16_t start_x;
    uint16_t start_y;
    uint16_t end_x;
    uint16_t end_y;
} watch_input_touch_t;

typedef struct
{
    bool raw_pressed[WATCH_INPUT_BUTTON_COUNT];
    bool stable_pressed[WATCH_INPUT_BUTTON_COUNT];
    uint32_t raw_since_ms[WATCH_INPUT_BUTTON_COUNT];
    watch_event_t event_queue[WATCH_INPUT_EVENT_CAPACITY];
    uint8_t event_head;
    uint8_t event_tail;
    uint8_t event_count;
} watch_input_t;

bool watch_input_init(watch_input_t *input);
bool watch_input_seed_button(watch_input_t *input, watch_input_button_t button, bool pressed,
                             uint32_t now_ms);
bool watch_input_submit_button(watch_input_t *input, watch_input_button_t button, bool pressed,
                               uint32_t now_ms);
bool watch_input_submit_encoder(watch_input_t *input, int16_t delta);
bool watch_input_submit_touch(watch_input_t *input, const watch_input_touch_t *touch);
bool watch_input_take_event(watch_input_t *input, watch_event_t *event);

#endif /* WATCH_INPUT_H */
