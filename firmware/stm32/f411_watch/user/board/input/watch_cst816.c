#include "watch_cst816.h"

#include "main.h"

#define WATCH_CST816_DELAY_LOOPS 160U
#define WATCH_CST816_RESET_LOW_MS 10U
#define WATCH_CST816_RESET_SETTLE_MS 100U

#define WATCH_CST816_REG_GESTURE 0x01U
#define WATCH_CST816_REG_CHIP_ID 0xA7U

static bool s_ready;
static uint8_t s_chip_id;
static uint32_t s_error_count;

static void cst816_delay(void)
{
    volatile uint32_t index;

    for (index = 0U; index < WATCH_CST816_DELAY_LOOPS; ++index) {
        __NOP();
    }
}

static void cst816_sda_release(void)
{
    HAL_GPIO_WritePin(TP_SDA_GPIO_Port, TP_SDA_Pin, GPIO_PIN_SET);
}

static void cst816_sda_low(void)
{
    HAL_GPIO_WritePin(TP_SDA_GPIO_Port, TP_SDA_Pin, GPIO_PIN_RESET);
}

static void cst816_scl_release(void)
{
    HAL_GPIO_WritePin(TP_SCL_GPIO_Port, TP_SCL_Pin, GPIO_PIN_SET);
}

static void cst816_scl_low(void)
{
    HAL_GPIO_WritePin(TP_SCL_GPIO_Port, TP_SCL_Pin, GPIO_PIN_RESET);
}

static bool cst816_sda_is_high(void)
{
    return HAL_GPIO_ReadPin(TP_SDA_GPIO_Port, TP_SDA_Pin) == GPIO_PIN_SET;
}

static void cst816_start(void)
{
    cst816_sda_release();
    cst816_scl_release();
    cst816_delay();
    cst816_sda_low();
    cst816_delay();
    cst816_scl_low();
    cst816_delay();
}

static void cst816_stop(void)
{
    cst816_sda_low();
    cst816_scl_release();
    cst816_delay();
    cst816_sda_release();
    cst816_delay();
}

static bool cst816_write_byte(uint8_t byte)
{
    uint8_t bit;
    bool acknowledged;

    for (bit = 0U; bit < 8U; ++bit) {
        if ((byte & 0x80U) != 0U) {
            cst816_sda_release();
        } else {
            cst816_sda_low();
        }
        cst816_scl_release();
        cst816_delay();
        cst816_scl_low();
        cst816_delay();
        byte <<= 1U;
    }

    cst816_sda_release();
    cst816_scl_release();
    cst816_delay();
    acknowledged = !cst816_sda_is_high();
    cst816_scl_low();
    cst816_delay();
    return acknowledged;
}

static uint8_t cst816_read_byte(bool acknowledge)
{
    uint8_t bit;
    uint8_t byte = 0U;

    cst816_sda_release();
    for (bit = 0U; bit < 8U; ++bit) {
        byte <<= 1U;
        cst816_scl_release();
        cst816_delay();
        if (cst816_sda_is_high()) {
            byte |= 1U;
        }
        cst816_scl_low();
        cst816_delay();
    }

    if (acknowledge) {
        cst816_sda_low();
    } else {
        cst816_sda_release();
    }
    cst816_scl_release();
    cst816_delay();
    cst816_scl_low();
    cst816_sda_release();
    cst816_delay();
    return byte;
}

static bool cst816_read_registers(uint8_t reg, uint8_t *data, uint8_t length)
{
    bool success = true;
    uint8_t index;

    if ((data == 0) || (length == 0U)) {
        return false;
    }

    cst816_start();
    success = cst816_write_byte((uint8_t)(WATCH_CST816_I2C_ADDRESS << 1U));
    success = success && cst816_write_byte(reg);
    cst816_start();
    success = success && cst816_write_byte((uint8_t)((WATCH_CST816_I2C_ADDRESS << 1U) | 1U));

    for (index = 0U; index < length; ++index) {
        data[index] = cst816_read_byte(index + 1U < length);
    }
    cst816_stop();

    if (!success) {
        ++s_error_count;
    }
    return success;
}

static bool cst816_read_register(uint8_t reg, uint8_t *data)
{
    return cst816_read_registers(reg, data, 1U);
}

bool watch_cst816_init(void)
{
    uint8_t chip_id = 0U;

    s_ready = false;
    s_chip_id = 0U;
    s_error_count = 0U;

    cst816_sda_release();
    cst816_scl_release();
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(WATCH_CST816_RESET_LOW_MS);
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(WATCH_CST816_RESET_SETTLE_MS);

    if (!cst816_read_register(WATCH_CST816_REG_CHIP_ID, &chip_id)) {
        return false;
    }

    s_chip_id = chip_id;
    s_ready = true;
    return true;
}

bool watch_cst816_read(watch_cst816_sample_t *sample)
{
    uint8_t data[6];

    if ((sample == 0) || !s_ready
        || !cst816_read_registers(WATCH_CST816_REG_GESTURE, data, (uint8_t)sizeof(data))) {
        return false;
    }

    sample->gesture = data[0];
    sample->finger_count = data[1];
    sample->x = (uint16_t)(((uint16_t)(data[2] & 0x0FU) << 8U) | data[3]);
    sample->y = (uint16_t)(((uint16_t)(data[4] & 0x0FU) << 8U) | data[5]);
    return true;
}

bool watch_cst816_is_ready(void)
{
    return s_ready;
}

uint8_t watch_cst816_chip_id(void)
{
    return s_chip_id;
}

uint32_t watch_cst816_error_count(void)
{
    return s_error_count;
}
