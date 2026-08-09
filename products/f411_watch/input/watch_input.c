#include "watch_input.h"

#include <stddef.h>

static bool watch_input_has_event_space(const watch_input_t *input)
{
    return input->event_count < WATCH_INPUT_EVENT_CAPACITY;
}

static bool watch_input_enqueue_event(watch_input_t *input, watch_event_type_t type)
{
    if (!watch_input_has_event_space(input)) {
        return false;
    }

    input->event_queue[input->event_head] = (watch_event_t) { .type = type };
    input->event_head = (uint8_t)((input->event_head + 1U) % WATCH_INPUT_EVENT_CAPACITY);
    input->event_count++;
    return true;
}

static watch_event_type_t watch_input_button_event(watch_input_button_t button)
{
    switch (button) {
    case WATCH_INPUT_BUTTON_BACK:
        return WATCH_EVENT_BACK;
    case WATCH_INPUT_BUTTON_WAKE:
        return WATCH_EVENT_WAKE;
    case WATCH_INPUT_BUTTON_ENCODER:
        return WATCH_EVENT_SELECT;
    case WATCH_INPUT_BUTTON_COUNT:
        return WATCH_EVENT_NONE;
    }

    return WATCH_EVENT_NONE;
}

static int32_t watch_input_abs(int32_t value)
{
    return value < 0 ? -value : value;
}

bool watch_input_init(watch_input_t *input)
{
    if (input == NULL) {
        return false;
    }

    *input = (watch_input_t) { 0 };
    return true;
}

bool watch_input_seed_button(watch_input_t *input, watch_input_button_t button, bool pressed,
                             uint32_t now_ms)
{
    if (input == NULL || button >= WATCH_INPUT_BUTTON_COUNT) {
        return false;
    }

    input->raw_pressed[button] = pressed;
    input->stable_pressed[button] = pressed;
    input->raw_since_ms[button] = now_ms;
    return true;
}

bool watch_input_submit_button(watch_input_t *input, watch_input_button_t button, bool pressed,
                               uint32_t now_ms)
{
    if (input == NULL || button >= WATCH_INPUT_BUTTON_COUNT) {
        return false;
    }

    if (pressed != input->raw_pressed[button]) {
        input->raw_pressed[button] = pressed;
        input->raw_since_ms[button] = now_ms;
    }

    if (pressed == input->stable_pressed[button]
        || (now_ms - input->raw_since_ms[button]) < WATCH_INPUT_BUTTON_DEBOUNCE_MS) {
        return true;
    }

    if (pressed) {
        watch_event_type_t event_type = watch_input_button_event(button);

        if (!watch_input_enqueue_event(input, event_type)) {
            return false;
        }
    }

    input->stable_pressed[button] = pressed;
    return true;
}

bool watch_input_submit_encoder(watch_input_t *input, int16_t delta)
{
    if (input == NULL) {
        return false;
    }

    if (delta == 0) {
        return true;
    }

    return watch_input_enqueue_event(input, delta > 0 ? WATCH_EVENT_DOWN : WATCH_EVENT_UP);
}

bool watch_input_submit_touch(watch_input_t *input, const watch_input_touch_t *touch)
{
    int32_t delta_x;
    int32_t delta_y;
    int32_t distance_x;
    int32_t distance_y;

    if (input == NULL || touch == NULL) {
        return false;
    }

    delta_x = (int32_t)touch->end_x - (int32_t)touch->start_x;
    delta_y = (int32_t)touch->end_y - (int32_t)touch->start_y;
    distance_x = watch_input_abs(delta_x);
    distance_y = watch_input_abs(delta_y);

    if (distance_x < (int32_t)WATCH_INPUT_TOUCH_SWIPE_MIN_PX
        && distance_y < (int32_t)WATCH_INPUT_TOUCH_SWIPE_MIN_PX) {
        return watch_input_enqueue_event(input, WATCH_EVENT_SELECT);
    }

    if (delta_x >= (int32_t)WATCH_INPUT_TOUCH_SWIPE_MIN_PX
        && touch->start_x <= WATCH_INPUT_TOUCH_EDGE_PX && distance_x >= distance_y) {
        return watch_input_enqueue_event(input, WATCH_EVENT_BACK);
    }

    if (distance_y >= (int32_t)WATCH_INPUT_TOUCH_SWIPE_MIN_PX && distance_y > distance_x) {
        return watch_input_enqueue_event(input, delta_y < 0 ? WATCH_EVENT_UP : WATCH_EVENT_DOWN);
    }

    return true;
}

bool watch_input_take_event(watch_input_t *input, watch_event_t *event)
{
    if (input == NULL || event == NULL || input->event_count == 0U) {
        return false;
    }

    *event = input->event_queue[input->event_tail];
    input->event_tail = (uint8_t)((input->event_tail + 1U) % WATCH_INPUT_EVENT_CAPACITY);
    input->event_count--;
    return true;
}
