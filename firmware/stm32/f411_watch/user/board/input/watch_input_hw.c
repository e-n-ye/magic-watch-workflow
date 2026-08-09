#include "watch_input_hw.h"

#include "main.h"
#include "tim.h"
#include "watch_cst816.h"
#include "watch_input.h"

#define WATCH_INPUT_HW_ENCODER_ORIGIN 32768U
#define WATCH_INPUT_HW_ENCODER_SAMPLE_MS 30U
#define WATCH_INPUT_HW_ENCODER_MIN_COUNTS 2
#define WATCH_INPUT_HW_ENCODER_CONFIRMATIONS 2U
#define WATCH_INPUT_HW_TOUCH_POLL_MS 10U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_high;
    watch_input_button_t button;
} watch_input_hw_button_t;

static const watch_input_hw_button_t s_buttons[WATCH_INPUT_HW_BUTTON_COUNT] = {
    { KEY_BACK_GPIO_Port, KEY_BACK_Pin, false, WATCH_INPUT_BUTTON_BACK },
    { KEY_WAKE_GPIO_Port, KEY_WAKE_Pin, true, WATCH_INPUT_BUTTON_WAKE },
    { ENCODER_KEY_GPIO_Port, ENCODER_KEY_Pin, false, WATCH_INPUT_BUTTON_ENCODER },
};

static watch_input_t s_input;
static bool s_initialized;
static bool s_encoder_ready;
static uint16_t s_encoder_last_count;
static uint32_t s_encoder_last_sample_ms;
static int8_t s_encoder_last_direction;
static uint8_t s_encoder_direction_confirmations;
static uint32_t s_last_touch_poll_ms;
static bool s_touch_gesture_active;
static uint8_t s_touch_gesture;
static uint8_t s_touch_finger_count;
static uint16_t s_touch_x;
static uint16_t s_touch_y;
static watch_event_type_t s_touch_event;
static bool s_touch_event_queued;
static uint32_t s_touch_sequence;
static volatile uint32_t s_exti_count[WATCH_INPUT_HW_BUTTON_COUNT];
static uint32_t s_event_dropped;

static bool input_pin_is_pressed(const watch_input_hw_button_t *button)
{
    bool high = HAL_GPIO_ReadPin(button->port, button->pin) == GPIO_PIN_SET;
    return high == button->active_high;
}

static bool input_submit_button(const watch_input_hw_button_t *button, bool pressed,
                                uint32_t now_ms)
{
    if (!watch_input_submit_button(&s_input, button->button, pressed, now_ms)) {
        ++s_event_dropped;
        return false;
    }
    return true;
}

static void input_process_buttons(uint32_t now_ms)
{
    uint8_t index;

    for (index = 0U; index < WATCH_INPUT_HW_BUTTON_COUNT; ++index) {
        (void)input_submit_button(&s_buttons[index], input_pin_is_pressed(&s_buttons[index]),
                                  now_ms);
    }
}

static bool input_encoder_step(uint32_t now_ms, int16_t *step)
{
    uint16_t current_count;
    int16_t delta;
    int8_t direction;

    if (!s_encoder_ready
        || (now_ms - s_encoder_last_sample_ms) < WATCH_INPUT_HW_ENCODER_SAMPLE_MS) {
        return false;
    }

    current_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    delta = (int16_t)(current_count - s_encoder_last_count);
    if ((delta > -WATCH_INPUT_HW_ENCODER_MIN_COUNTS)
        && (delta < WATCH_INPUT_HW_ENCODER_MIN_COUNTS)) {
        return false;
    }

    direction = delta > 0 ? 1 : -1;
#if WATCH_INPUT_HW_ENCODER_REVERSE
    direction = (int8_t)-direction;
#endif
    if (direction != s_encoder_last_direction) {
        s_encoder_last_direction = direction;
        s_encoder_direction_confirmations = 1U;
        return false;
    }

    if (s_encoder_direction_confirmations < WATCH_INPUT_HW_ENCODER_CONFIRMATIONS) {
        ++s_encoder_direction_confirmations;
    }

    if (s_encoder_direction_confirmations < WATCH_INPUT_HW_ENCODER_CONFIRMATIONS) {
        return false;
    }

    s_encoder_last_count = current_count;
    s_encoder_last_sample_ms = now_ms;
    s_encoder_direction_confirmations = 0U;
    *step = direction;
    return true;
}

static bool input_submit_touch_sample(const watch_cst816_sample_t *sample)
{
    watch_input_touch_t touch;
    watch_event_type_t expected_event = WATCH_EVENT_NONE;

    switch (sample->gesture) {
    case WATCH_CST816_GESTURE_CLICK:
        expected_event = WATCH_EVENT_SELECT;
        touch = (watch_input_touch_t) {
            .start_x = sample->x,
            .start_y = sample->y,
            .end_x = sample->x,
            .end_y = sample->y,
        };
        break;
    case WATCH_CST816_GESTURE_SLIDE_RIGHT:
        expected_event = WATCH_EVENT_BACK;
        /* CST816 polling exposes the gesture but not its initial coordinate. */
        touch = (watch_input_touch_t) {
            .start_x = 0U,
            .start_y = sample->y,
            .end_x = WATCH_INPUT_TOUCH_SWIPE_MIN_PX,
            .end_y = sample->y,
        };
        break;
    case WATCH_CST816_GESTURE_SLIDE_UP:
        expected_event = WATCH_EVENT_UP;
        touch = (watch_input_touch_t) {
            .start_x = sample->x,
            .start_y = WATCH_INPUT_TOUCH_SWIPE_MIN_PX,
            .end_x = sample->x,
            .end_y = 0U,
        };
        break;
    case WATCH_CST816_GESTURE_SLIDE_DOWN:
        expected_event = WATCH_EVENT_DOWN;
        touch = (watch_input_touch_t) {
            .start_x = sample->x,
            .start_y = 0U,
            .end_x = sample->x,
            .end_y = WATCH_INPUT_TOUCH_SWIPE_MIN_PX,
        };
        break;
    default:
        s_touch_event = WATCH_EVENT_NONE;
        s_touch_event_queued = false;
        return true;
    }

    s_touch_event = expected_event;
    if (!watch_input_submit_touch(&s_input, &touch)) {
        ++s_event_dropped;
        s_touch_event_queued = false;
        return false;
    }
    s_touch_event_queued = true;
    return true;
}

static void input_process_touch(uint32_t now_ms)
{
    watch_cst816_sample_t sample;

    if (!watch_cst816_is_ready()
        || (now_ms - s_last_touch_poll_ms) < WATCH_INPUT_HW_TOUCH_POLL_MS) {
        return;
    }

    s_last_touch_poll_ms = now_ms;
    if (!watch_cst816_read(&sample)) {
        return;
    }

    if (sample.gesture == WATCH_CST816_GESTURE_NONE) {
        s_touch_gesture_active = false;
        return;
    }

    if (s_touch_gesture_active) {
        return;
    }

    s_touch_gesture_active = true;
    s_touch_gesture = sample.gesture;
    s_touch_finger_count = sample.finger_count;
    s_touch_x = sample.x;
    s_touch_y = sample.y;
    s_touch_event = WATCH_EVENT_NONE;
    s_touch_event_queued = false;
    ++s_touch_sequence;
    (void)input_submit_touch_sample(&sample);
}

bool watch_input_hw_init(void)
{
    uint32_t now_ms = HAL_GetTick();
    uint8_t index;

    s_initialized = false;
    s_encoder_ready = false;
    s_encoder_last_count = WATCH_INPUT_HW_ENCODER_ORIGIN;
    s_encoder_last_sample_ms = now_ms;
    s_encoder_last_direction = 0;
    s_encoder_direction_confirmations = 0U;
    s_last_touch_poll_ms = now_ms;
    s_touch_gesture_active = false;
    s_touch_gesture = WATCH_CST816_GESTURE_NONE;
    s_touch_finger_count = 0U;
    s_touch_x = 0U;
    s_touch_y = 0U;
    s_touch_event = WATCH_EVENT_NONE;
    s_touch_event_queued = false;
    s_touch_sequence = 0U;
    s_event_dropped = 0U;
    for (index = 0U; index < WATCH_INPUT_HW_BUTTON_COUNT; ++index) {
        s_exti_count[index] = 0U;
    }

    if (!watch_input_init(&s_input)) {
        return false;
    }

    for (index = 0U; index < WATCH_INPUT_HW_BUTTON_COUNT; ++index) {
        (void)watch_input_seed_button(&s_input, s_buttons[index].button,
                                      input_pin_is_pressed(&s_buttons[index]), now_ms);
    }

    if (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) == HAL_OK) {
        __HAL_TIM_SET_COUNTER(&htim4, WATCH_INPUT_HW_ENCODER_ORIGIN);
        s_encoder_last_count = WATCH_INPUT_HW_ENCODER_ORIGIN;
        s_encoder_ready = true;
    }

    (void)watch_cst816_init();
    s_initialized = true;
    return s_encoder_ready;
}

void watch_input_hw_process(uint32_t now_ms)
{
    int16_t encoder_step;

    if (!s_initialized) {
        return;
    }

    input_process_buttons(now_ms);

    if (input_encoder_step(now_ms, &encoder_step)
        && !watch_input_submit_encoder(&s_input, encoder_step)) {
        ++s_event_dropped;
    }

    input_process_touch(now_ms);
}

bool watch_input_hw_take_event(watch_event_t *event)
{
    if (!s_initialized) {
        return false;
    }
    return watch_input_take_event(&s_input, event);
}

void watch_input_hw_read_status(watch_input_hw_status_t *status)
{
    uint8_t index;

    if (status == 0) {
        return;
    }

    status->encoder_ready = s_encoder_ready;
    status->touch_ready = watch_cst816_is_ready();
    status->touch_chip_id = watch_cst816_chip_id();
    status->encoder_count = s_encoder_ready ? (uint16_t)__HAL_TIM_GET_COUNTER(&htim4) : 0U;
    status->touch_errors = watch_cst816_error_count();
    status->event_dropped = s_event_dropped;
    status->touch_gesture = s_touch_gesture;
    status->touch_finger_count = s_touch_finger_count;
    status->touch_x = s_touch_x;
    status->touch_y = s_touch_y;
    status->touch_event = s_touch_event;
    status->touch_event_queued = s_touch_event_queued;
    status->touch_sequence = s_touch_sequence;

    __disable_irq();
    for (index = 0U; index < WATCH_INPUT_HW_BUTTON_COUNT; ++index) {
        status->exti_count[index] = s_exti_count[index];
    }
    __enable_irq();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_BACK_Pin) {
        ++s_exti_count[0];
    } else if (GPIO_Pin == KEY_WAKE_Pin) {
        ++s_exti_count[1];
    } else if (GPIO_Pin == ENCODER_KEY_Pin) {
        ++s_exti_count[2];
    }
}
