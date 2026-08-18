#include "board/bluetooth/watch_kt6368_board.h"

#include <string.h>

#include "main.h"
#include "usart.h"

#define WATCH_KT6368_ENABLE_SETTLE_MS 100U
#define WATCH_KT6368_RECOVERY_DELAY_MS 100U

static watch_spp_transport_t s_transport;
static DMA_HandleTypeDef s_hdma_tx;
static uint8_t s_dma_buffer[WATCH_KT6368_DMA_BUFFER_SIZE];
static uint8_t s_tx_buffer[WATCH_SPP_TX_CHUNK_SIZE];
static bool s_initialized;
static bool s_recovery_pending;
static bool s_tx_started;
static uint32_t s_recovery_due_ms;
static uint32_t s_uart_error_count;
static uint32_t s_tx_busy_count;
static uint32_t s_last_error_ms;

static bool init_tx_dma(void)
{
    if (huart1.hdmatx != NULL) {
        return true;
    }

    memset(&s_hdma_tx, 0, sizeof(s_hdma_tx));
    s_hdma_tx.Instance = DMA2_Stream7;
    s_hdma_tx.Init.Channel = DMA_CHANNEL_4;
    s_hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    s_hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    s_hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
    s_hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    s_hdma_tx.Init.Mode = DMA_NORMAL;
    s_hdma_tx.Init.Priority = DMA_PRIORITY_LOW;
    s_hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&s_hdma_tx) != HAL_OK) {
        return false;
    }

    __HAL_LINKDMA(&huart1, hdmatx, s_hdma_tx);
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    return true;
}

static bool arm_rx(void)
{
    HAL_StatusTypeDef result;

    HAL_UART_DMAStop(&huart1);
    result = HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_dma_buffer, sizeof(s_dma_buffer));
    if (result == HAL_OK && huart1.hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    }
    watch_spp_transport_set_rx_armed(&s_transport, result == HAL_OK);
    return result == HAL_OK;
}

static void stop_uart(void)
{
    HAL_UART_DMAStop(&huart1);
    s_tx_started = false;
    watch_spp_transport_set_rx_armed(&s_transport, false);
}

static void request_recovery(uint32_t now_ms)
{
    if (!s_initialized || s_recovery_pending) {
        return;
    }

    s_recovery_pending = true;
    s_recovery_due_ms = now_ms + WATCH_KT6368_RECOVERY_DELAY_MS;
    stop_uart();
}

static bool restart_after_recovery(uint32_t now_ms)
{
    if (!s_recovery_pending || (int32_t)(now_ms - s_recovery_due_ms) < 0) {
        return false;
    }

    HAL_GPIO_WritePin(BLE_EN_GPIO_Port, BLE_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(WATCH_KT6368_ENABLE_SETTLE_MS);
    HAL_GPIO_WritePin(BLE_EN_GPIO_Port, BLE_EN_Pin, GPIO_PIN_RESET);
    watch_spp_transport_on_recovery(&s_transport);
    s_recovery_pending = false;
    return arm_rx();
}

bool watch_kt6368_board_init(void)
{
    if (s_initialized) {
        return true;
    }

    if (!init_tx_dma()) {
        return false;
    }

    watch_spp_transport_init(&s_transport);
    HAL_GPIO_WritePin(BLE_EN_GPIO_Port, BLE_EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(WATCH_KT6368_ENABLE_SETTLE_MS);
    watch_spp_transport_set_enabled(&s_transport, true);
    s_initialized = true;
    s_recovery_pending = false;
    s_tx_started = false;
    return arm_rx();
}

void watch_kt6368_board_process(uint32_t now_ms)
{
    const uint8_t *data;
    size_t length;
    HAL_StatusTypeDef result;

    if (!watch_kt6368_board_init()) {
        return;
    }

    if (s_recovery_pending) {
        (void)restart_after_recovery(now_ms);
        return;
    }

    watch_spp_transport_process(&s_transport, now_ms);
    if (s_tx_started) {
        return;
    }

    length = watch_spp_transport_tx_peek(&s_transport, &data);
    if (length == 0U) {
        return;
    }
    memcpy(s_tx_buffer, data, length);
    result = HAL_UART_Transmit_DMA(&huart1, s_tx_buffer, (uint16_t)length);
    if (result == HAL_OK) {
        s_tx_started = true;
        watch_spp_transport_on_tx_started(&s_transport, length);
    } else if (result == HAL_BUSY) {
        ++s_tx_busy_count;
    } else {
        watch_spp_transport_on_tx_started(&s_transport, length);
        watch_spp_transport_on_tx_complete(&s_transport, false);
        request_recovery(now_ms);
    }
}

bool watch_kt6368_board_set_enabled(bool enabled)
{
    if (!watch_kt6368_board_init()) {
        return false;
    }

    stop_uart();
    HAL_GPIO_WritePin(BLE_EN_GPIO_Port, BLE_EN_Pin, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
    watch_spp_transport_set_enabled(&s_transport, enabled);
    s_recovery_pending = false;
    if (!enabled) {
        return true;
    }

    HAL_Delay(WATCH_KT6368_ENABLE_SETTLE_MS);
    return arm_rx();
}

size_t watch_kt6368_board_read(uint8_t *data, size_t length)
{
    size_t result;

    __disable_irq();
    result = watch_spp_transport_read(&s_transport, data, length);
    __enable_irq();
    return result;
}

size_t watch_kt6368_board_write(const uint8_t *data, size_t length)
{
    size_t result;

    __disable_irq();
    result = watch_spp_transport_write(&s_transport, data, length);
    __enable_irq();
    return result;
}

bool watch_kt6368_board_read_status(watch_kt6368_board_status_t *status)
{
    if (status == NULL) {
        return false;
    }

    __disable_irq();
    status->initialized = s_initialized;
    status->enable_active_low = true;
    status->recovery_pending = s_recovery_pending;
    status->uart_error_count = s_uart_error_count;
    status->tx_busy_count = s_tx_busy_count;
    status->last_error_ms = s_last_error_ms;
    (void)watch_spp_transport_read_status(&s_transport, &status->transport);
    __enable_irq();
    return true;
}

void watch_kt6368_board_on_rx_event(const UART_HandleTypeDef *huart, uint16_t size)
{
    if (!s_initialized || huart != &huart1) {
        return;
    }

    if (size > sizeof(s_dma_buffer)) {
        size = sizeof(s_dma_buffer);
    }
    watch_spp_transport_on_dma_idle(&s_transport, s_dma_buffer, size, HAL_GetTick());
    (void)arm_rx();
}

void watch_kt6368_board_on_tx_complete(const UART_HandleTypeDef *huart)
{
    if (!s_initialized || huart != &huart1 || !s_tx_started) {
        return;
    }

    watch_spp_transport_on_tx_complete(&s_transport, true);
    s_tx_started = false;
}

void watch_kt6368_board_on_error(const UART_HandleTypeDef *huart)
{
    if (!s_initialized || huart != &huart1) {
        return;
    }

    ++s_uart_error_count;
    s_last_error_ms = HAL_GetTick();
    watch_spp_transport_on_rx_error(&s_transport);
    request_recovery(s_last_error_ms);
}

void watch_kt6368_board_dma_tx_irq(void)
{
    if (s_initialized && huart1.hdmatx != NULL) {
        HAL_DMA_IRQHandler(huart1.hdmatx);
    }
}

/* cppcheck-suppress constParameterPointer */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    watch_kt6368_board_on_rx_event(huart, size);
}

/* cppcheck-suppress constParameterPointer */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    watch_kt6368_board_on_tx_complete(huart);
}

/* cppcheck-suppress constParameterPointer */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    watch_kt6368_board_on_error(huart);
}
