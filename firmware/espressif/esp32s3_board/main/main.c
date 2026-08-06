#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#define RGB_LED_GPIO 48
#define COLOR_HOLD_MS 1000

static const char *TAG = "rgb_blink";

typedef struct {
    const char *name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_color_t;

static const rgb_color_t COLORS[] = {
    {.name = "red", .red = 32, .green = 0, .blue = 0},
    {.name = "green", .red = 0, .green = 32, .blue = 0},
    {.name = "blue", .red = 0, .green = 0, .blue = 32},
    {.name = "off", .red = 0, .green = 0, .blue = 0},
};

void app_main(void)
{
    led_strip_handle_t strip = NULL;
    const led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO,
        .max_leds = 1,
    };
    const led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));
    ESP_ERROR_CHECK(led_strip_clear(strip));
    ESP_LOGI(TAG, "RGB blink started on GPIO%d", RGB_LED_GPIO);

    while (true) {
        for (size_t index = 0; index < sizeof(COLORS) / sizeof(COLORS[0]); ++index) {
            const rgb_color_t *color = &COLORS[index];

            ESP_LOGI(TAG, "color=%s", color->name);
            ESP_ERROR_CHECK(led_strip_set_pixel(
                strip, 0, color->red, color->green, color->blue));
            ESP_ERROR_CHECK(led_strip_refresh(strip));
            vTaskDelay(pdMS_TO_TICKS(COLOR_HOLD_MS));
        }
    }
}
