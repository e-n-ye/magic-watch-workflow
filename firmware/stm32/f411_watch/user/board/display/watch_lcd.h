#ifndef WATCH_LCD_H
#define WATCH_LCD_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_LCD_BLACK   0x0000U
#define WATCH_LCD_WHITE   0xFFFFU
#define WATCH_LCD_RED     0xF800U
#define WATCH_LCD_GREEN   0x07E0U
#define WATCH_LCD_BLUE    0x001FU
#define WATCH_LCD_YELLOW  0xFFE0U
#define WATCH_LCD_CYAN    0x07FFU
#define WATCH_LCD_MAGENTA 0xF81FU
#define WATCH_LCD_GRAY    0x8410U

typedef void (*watch_lcd_transfer_done_cb_t)(void *context);

typedef enum {
    WATCH_LCD_TRANSFER_FAILED = 0,
    WATCH_LCD_TRANSFER_BLOCKING_DONE,
    WATCH_LCD_TRANSFER_DMA_STARTED
} watch_lcd_transfer_result_t;

void watch_lcd_init(void);
void watch_lcd_backlight_on(void);
void watch_lcd_backlight_set(uint8_t percent);
void watch_lcd_fill(uint16_t color);
void watch_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void watch_lcd_draw_rgb565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels);
void watch_lcd_draw_rgb565_bytes_blocking(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *bytes,
    uint16_t byte_count);
watch_lcd_transfer_result_t watch_lcd_draw_rgb565_bytes(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *bytes,
    uint16_t byte_count,
    watch_lcd_transfer_done_cb_t done_cb,
    void *context);
bool watch_lcd_dma_is_busy(void);
bool watch_lcd_dma_consume_error(void);
void watch_lcd_show_bringup_pattern(void);

#endif /* WATCH_LCD_H */
