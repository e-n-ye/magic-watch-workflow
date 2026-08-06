#include "watch_lcd.h"

#include "config/user_config.h"
#include "main.h"
#include "spi.h"
#include "tim.h"

#define LCD_SPI_TIMEOUT_MS 100U
#define LCD_OFFSET_X       0U
#define LCD_OFFSET_Y       20U
#define LCD_BL_PWM_MAX     999U
#define LCD_LINE_BUF_BYTES (WATCH_LCD_WIDTH * 2U)
#define LCD_DMA_MIN_BYTES  128U

static const uint8_t kGammaPositive[] = {
    0xD0U, 0x04U, 0x0DU, 0x11U, 0x13U, 0x2BU, 0x3FU,
    0x54U, 0x4CU, 0x18U, 0x0DU, 0x0BU, 0x1FU, 0x23U,
};

static const uint8_t kGammaNegative[] = {
    0xD0U, 0x04U, 0x0CU, 0x11U, 0x13U, 0x2CU, 0x3FU,
    0x44U, 0x51U, 0x2FU, 0x1FU, 0x1FU, 0x20U, 0x23U,
};

static uint8_t s_line_buf[LCD_LINE_BUF_BYTES];
static volatile uint8_t s_dma_transfer_active;
static volatile uint8_t s_dma_transfer_error;
static watch_lcd_transfer_done_cb_t s_dma_done_cb;
static void *s_dma_done_context;

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

static void lcd_select(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

static void lcd_unselect(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static void lcd_write_bytes(const uint8_t *data, uint16_t size)
{
    (void)HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size, LCD_SPI_TIMEOUT_MS);
}

static void lcd_write_cmd(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    lcd_write_bytes(&cmd, 1U);
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
}

static void lcd_write_data8(uint8_t data)
{
    lcd_write_bytes(&data, 1U);
}

static void lcd_write_data16(uint16_t data)
{
    uint8_t bytes[2];
    bytes[0] = (uint8_t)(data >> 8);
    bytes[1] = (uint8_t)(data & 0xFFU);
    lcd_write_bytes(bytes, 2U);
}

static bool lcd_dma_available(void)
{
    return hspi1.hdmatx != 0;
}

static bool lcd_area_is_valid(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if ((width == 0U) || (height == 0U) || (x >= WATCH_LCD_WIDTH) || (y >= WATCH_LCD_HEIGHT)) {
        return false;
    }

    if (((uint32_t)x + (uint32_t)width) > (uint32_t)WATCH_LCD_WIDTH) {
        return false;
    }

    if (((uint32_t)y + (uint32_t)height) > (uint32_t)WATCH_LCD_HEIGHT) {
        return false;
    }

    return true;
}

static bool lcd_rgb565_byte_count_matches(uint16_t width, uint16_t height, uint16_t byte_count)
{
    const uint32_t expected = (uint32_t)width * (uint32_t)height * 2U;
    return expected <= UINT16_MAX && byte_count == (uint16_t)expected;
}

static void lcd_draw_rgb565_bytes_blocking(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *bytes,
    uint16_t byte_count)
{
    lcd_select();
    lcd_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
    lcd_write_bytes(bytes, byte_count);
    lcd_unselect();
}

void watch_lcd_draw_rgb565_bytes_blocking(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *bytes,
    uint16_t byte_count)
{
    if ((bytes == 0) ||
        !lcd_area_is_valid(x, y, width, height) ||
        !lcd_rgb565_byte_count_matches(width, height, byte_count)) {
        return;
    }

    lcd_draw_rgb565_bytes_blocking(x, y, width, height, bytes, byte_count);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_write_cmd(0x2AU);
    lcd_write_data16((uint16_t)(x0 + LCD_OFFSET_X));
    lcd_write_data16((uint16_t)(x1 + LCD_OFFSET_X));

    lcd_write_cmd(0x2BU);
    lcd_write_data16((uint16_t)(y0 + LCD_OFFSET_Y));
    lcd_write_data16((uint16_t)(y1 + LCD_OFFSET_Y));

    lcd_write_cmd(0x2CU);
}

void watch_lcd_init(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

    lcd_select();

    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100U);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100U);

    lcd_write_cmd(0x11U);
    HAL_Delay(120U);

    lcd_write_cmd(0x36U);
    lcd_write_data8(0x00U);

    lcd_write_cmd(0x3AU);
    lcd_write_data8(0x05U);

    lcd_write_cmd(0xB2U);
    lcd_write_data8(0x0CU);
    lcd_write_data8(0x0CU);
    lcd_write_data8(0x00U);
    lcd_write_data8(0x33U);
    lcd_write_data8(0x33U);

    lcd_write_cmd(0xB7U);
    lcd_write_data8(0x35U);

    lcd_write_cmd(0xBBU);
    lcd_write_data8(0x19U);

    lcd_write_cmd(0xC0U);
    lcd_write_data8(0x2CU);

    lcd_write_cmd(0xC2U);
    lcd_write_data8(0x01U);

    lcd_write_cmd(0xC3U);
    lcd_write_data8(0x12U);

    lcd_write_cmd(0xC4U);
    lcd_write_data8(0x20U);

    lcd_write_cmd(0xC6U);
    lcd_write_data8(0x0FU);

    lcd_write_cmd(0xD0U);
    lcd_write_data8(0xA4U);
    lcd_write_data8(0xA1U);

    lcd_write_cmd(0xE0U);
    lcd_write_bytes(kGammaPositive, (uint16_t)sizeof(kGammaPositive));

    lcd_write_cmd(0xE1U);
    lcd_write_bytes(kGammaNegative, (uint16_t)sizeof(kGammaNegative));

    lcd_write_cmd(0x21U);
    lcd_write_cmd(0x29U);

    lcd_unselect();
}

void watch_lcd_backlight_on(void)
{
    watch_lcd_backlight_set(100U);
    (void)HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
}

void watch_lcd_backlight_set(uint8_t percent)
{
    uint32_t duty;

    if (percent > 100U) {
        percent = 100U;
    }

    duty = (LCD_BL_PWM_MAX * (uint32_t)percent) / 100U;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
}

void watch_lcd_fill(uint16_t color)
{
    watch_lcd_fill_rect(0U, 0U, WATCH_LCD_WIDTH, WATCH_LCD_HEIGHT, color);
}

void watch_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t row;
    uint16_t column;
    uint16_t line_bytes;
    uint8_t color_hi;
    uint8_t color_lo;

    if ((width == 0U) || (height == 0U) || (x >= WATCH_LCD_WIDTH) || (y >= WATCH_LCD_HEIGHT)) {
        return;
    }

    if ((uint32_t)x + width > WATCH_LCD_WIDTH) {
        width = (uint16_t)(WATCH_LCD_WIDTH - x);
    }
    if ((uint32_t)y + height > WATCH_LCD_HEIGHT) {
        height = (uint16_t)(WATCH_LCD_HEIGHT - y);
    }

    color_hi = (uint8_t)(color >> 8);
    color_lo = (uint8_t)(color & 0xFFU);
    line_bytes = (uint16_t)(width * 2U);

    for (column = 0U; column < width; ++column) {
        s_line_buf[(uint16_t)(column * 2U)] = color_hi;
        s_line_buf[(uint16_t)(column * 2U + 1U)] = color_lo;
    }

    lcd_select();
    lcd_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));

    for (row = 0U; row < height; ++row) {
        lcd_write_bytes(s_line_buf, line_bytes);
    }

    lcd_unselect();
}

void watch_lcd_draw_rgb565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels)
{
    uint16_t row;
    uint16_t column;
    uint16_t line_bytes;
    uint16_t src_width;
    const uint16_t *src;
    uint16_t pixel;

    if ((pixels == 0) || (width == 0U) || (height == 0U) || (x >= WATCH_LCD_WIDTH) || (y >= WATCH_LCD_HEIGHT)) {
        return;
    }

    src_width = width;

    if ((uint32_t)x + width > WATCH_LCD_WIDTH) {
        width = (uint16_t)(WATCH_LCD_WIDTH - x);
    }
    if ((uint32_t)y + height > WATCH_LCD_HEIGHT) {
        height = (uint16_t)(WATCH_LCD_HEIGHT - y);
    }

    line_bytes = (uint16_t)(width * 2U);

    lcd_select();
    lcd_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));

    for (row = 0U; row < height; ++row) {
        src = &pixels[(uint32_t)row * src_width];
        for (column = 0U; column < width; ++column) {
            pixel = src[column];
            s_line_buf[(uint16_t)(column * 2U)] = (uint8_t)(pixel >> 8);
            s_line_buf[(uint16_t)(column * 2U + 1U)] = (uint8_t)(pixel & 0xFFU);
        }
        lcd_write_bytes(s_line_buf, line_bytes);
    }

    lcd_unselect();
}

watch_lcd_transfer_result_t watch_lcd_draw_rgb565_bytes(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *bytes,
    uint16_t byte_count,
    watch_lcd_transfer_done_cb_t done_cb,
    void *context)
{
    HAL_StatusTypeDef status;

    if ((bytes == 0) ||
        !lcd_area_is_valid(x, y, width, height) ||
        !lcd_rgb565_byte_count_matches(width, height, byte_count)) {
        return WATCH_LCD_TRANSFER_FAILED;
    }

    if (s_dma_transfer_active != 0U) {
        return WATCH_LCD_TRANSFER_FAILED;
    }

    if (!lcd_dma_available() ||
        (byte_count < LCD_DMA_MIN_BYTES) ||
        (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY)) {
        lcd_draw_rgb565_bytes_blocking(x, y, width, height, bytes, byte_count);
        return WATCH_LCD_TRANSFER_BLOCKING_DONE;
    }

    lcd_select();
    lcd_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));

    s_dma_done_cb = done_cb;
    s_dma_done_context = context;
    s_dma_transfer_error = 0U;
    s_dma_transfer_active = 1U;

    status = HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)bytes, byte_count);
    if (status != HAL_OK) {
        s_dma_transfer_active = 0U;
        s_dma_done_cb = 0;
        s_dma_done_context = 0;
        lcd_write_bytes(bytes, byte_count);
        lcd_unselect();
        return WATCH_LCD_TRANSFER_BLOCKING_DONE;
    }

    return WATCH_LCD_TRANSFER_DMA_STARTED;
}

bool watch_lcd_dma_is_busy(void)
{
    return s_dma_transfer_active != 0U;
}

bool watch_lcd_dma_consume_error(void)
{
    uint8_t had_error = s_dma_transfer_error;

    s_dma_transfer_error = 0U;
    return had_error != 0U;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    watch_lcd_transfer_done_cb_t done_cb;
    void *done_context;

    if ((hspi == 0) || (hspi->Instance != SPI1) || (s_dma_transfer_active == 0U)) {
        return;
    }

    done_cb = s_dma_done_cb;
    done_context = s_dma_done_context;
    s_dma_done_cb = 0;
    s_dma_done_context = 0;
    s_dma_transfer_error = 0U;
    s_dma_transfer_active = 0U;

    lcd_unselect();

    if (done_cb != 0) {
        done_cb(done_context);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if ((hspi == 0) || (hspi->Instance != SPI1) || (s_dma_transfer_active == 0U)) {
        return;
    }

    s_dma_done_cb = 0;
    s_dma_done_context = 0;
    s_dma_transfer_error = 1U;
    s_dma_transfer_active = 0U;
    lcd_unselect();
}

void watch_lcd_show_bringup_pattern(void)
{
    uint16_t stripe = (uint16_t)(WATCH_LCD_HEIGHT / 7U);

    watch_lcd_fill_rect(0U, (uint16_t)(0U * stripe), WATCH_LCD_WIDTH, stripe, WATCH_LCD_RED);
    watch_lcd_fill_rect(0U, (uint16_t)(1U * stripe), WATCH_LCD_WIDTH, stripe, WATCH_LCD_GREEN);
    watch_lcd_fill_rect(0U, (uint16_t)(2U * stripe), WATCH_LCD_WIDTH, stripe, WATCH_LCD_BLUE);
    watch_lcd_fill_rect(0U, (uint16_t)(3U * stripe), WATCH_LCD_WIDTH, stripe, WATCH_LCD_YELLOW);
    watch_lcd_fill_rect(0U, (uint16_t)(4U * stripe), WATCH_LCD_WIDTH, stripe, WATCH_LCD_CYAN);
    watch_lcd_fill_rect(0U, (uint16_t)(5U * stripe), WATCH_LCD_WIDTH, stripe, WATCH_LCD_MAGENTA);
    watch_lcd_fill_rect(0U, (uint16_t)(6U * stripe), WATCH_LCD_WIDTH, (uint16_t)(WATCH_LCD_HEIGHT - (6U * stripe)), WATCH_LCD_WHITE);
}
