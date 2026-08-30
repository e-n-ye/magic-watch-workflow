#ifndef WATCH_INPUT_HW_H
#define WATCH_INPUT_HW_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_core.h"

#define WATCH_INPUT_HW_BUTTON_COUNT 3U
#define WATCH_INPUT_HW_ENCODER_REVERSE 1U

typedef struct
{
    bool encoder_ready;
    bool touch_ready;
    uint8_t touch_chip_id;
    uint16_t encoder_count;
    uint32_t touch_errors;
    uint32_t exti_count[WATCH_INPUT_HW_BUTTON_COUNT];
    uint32_t event_dropped;
    uint8_t touch_gesture;
    uint8_t touch_finger_count;
    uint16_t touch_x;
    uint16_t touch_y;
    watch_event_type_t touch_event;
    bool touch_event_queued;
    uint32_t touch_sequence;
} watch_input_hw_status_t;

bool watch_input_hw_init(void);
void watch_input_hw_process(uint32_t now_ms);
bool watch_input_hw_take_event(watch_event_t *event);
bool watch_input_hw_read_touch(uint16_t *x, uint16_t *y, bool *pressed);
void watch_input_hw_read_status(watch_input_hw_status_t *status);

#endif /* WATCH_INPUT_HW_H */
