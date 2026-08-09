#ifndef WATCH_CST816_H
#define WATCH_CST816_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_CST816_I2C_ADDRESS 0x15U

#define WATCH_CST816_GESTURE_NONE 0x00U
#define WATCH_CST816_GESTURE_SLIDE_DOWN 0x01U
#define WATCH_CST816_GESTURE_SLIDE_UP 0x02U
#define WATCH_CST816_GESTURE_SLIDE_LEFT 0x03U
#define WATCH_CST816_GESTURE_SLIDE_RIGHT 0x04U
#define WATCH_CST816_GESTURE_CLICK 0x05U

typedef struct
{
    uint8_t gesture;
    uint8_t finger_count;
    uint16_t x;
    uint16_t y;
} watch_cst816_sample_t;

bool watch_cst816_init(void);
bool watch_cst816_read(watch_cst816_sample_t *sample);
bool watch_cst816_is_ready(void);
uint8_t watch_cst816_chip_id(void);
uint32_t watch_cst816_error_count(void);

#endif /* WATCH_CST816_H */
