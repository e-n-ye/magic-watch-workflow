#include "board/storage/watch_w25q128_board.h"

#include "main.h"
#include "spi.h"

#define WATCH_W25Q128_BOARD_SPI_TIMEOUT_MS 100U

static watch_w25q128_t s_device;
static bool s_initialized;

static bool watch_w25q128_board_transfer(void *context, const uint8_t *tx, uint8_t *rx,
                                         size_t length)
{
    SPI_HandleTypeDef *handle = (SPI_HandleTypeDef *)context;
    HAL_StatusTypeDef status;

    if (handle == NULL || tx == NULL || rx == NULL || length == 0U || length > UINT16_MAX) {
        return false;
    }

    HAL_GPIO_WritePin(W25Q128_CS_GPIO_Port, W25Q128_CS_Pin, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(handle, (uint8_t *)tx, rx, (uint16_t)length,
                                     WATCH_W25Q128_BOARD_SPI_TIMEOUT_MS);
    HAL_GPIO_WritePin(W25Q128_CS_GPIO_Port, W25Q128_CS_Pin, GPIO_PIN_SET);
    return status == HAL_OK;
}

static uint32_t watch_w25q128_board_now_ms(void *context)
{
    (void)context;
    return HAL_GetTick();
}

static void watch_w25q128_board_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    HAL_Delay(delay_ms);
}

bool watch_w25q128_board_init(void)
{
    watch_w25q128_bus_t bus;

    if (s_initialized) {
        return true;
    }

    bus = (watch_w25q128_bus_t) {
        .transfer = watch_w25q128_board_transfer,
        .now_ms = watch_w25q128_board_now_ms,
        .delay_ms = watch_w25q128_board_delay_ms,
        .context = &hspi3,
    };
    s_initialized = watch_w25q128_init(&s_device, &bus);
    return s_initialized;
}

watch_w25q128_result_t watch_w25q128_board_read_id(uint32_t *jedec_id)
{
    if (!watch_w25q128_board_init()) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    return watch_w25q128_read_id(&s_device, jedec_id);
}

watch_w25q128_result_t watch_w25q128_board_wait_ready(uint32_t timeout_ms)
{
    if (!watch_w25q128_board_init()) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    return watch_w25q128_wait_ready(&s_device, timeout_ms);
}

watch_w25q128_result_t watch_w25q128_board_read(uint32_t address, uint8_t *data, size_t length)
{
    if (!watch_w25q128_board_init()) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    return watch_w25q128_read(&s_device, address, data, length,
                              WATCH_W25Q128_BOARD_DEFAULT_TIMEOUT_MS);
}

watch_w25q128_result_t watch_w25q128_board_page_program(uint32_t address, const uint8_t *data,
                                                        size_t length)
{
    if (!watch_w25q128_board_init()) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    return watch_w25q128_page_program(&s_device, address, data, length,
                                      WATCH_W25Q128_BOARD_DEFAULT_TIMEOUT_MS);
}

watch_w25q128_result_t watch_w25q128_board_sector_erase(uint32_t address)
{
    if (!watch_w25q128_board_init()) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    return watch_w25q128_sector_erase(&s_device, address, WATCH_W25Q128_BOARD_DEFAULT_TIMEOUT_MS);
}
