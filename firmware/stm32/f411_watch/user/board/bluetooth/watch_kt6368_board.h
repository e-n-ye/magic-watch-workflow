#ifndef WATCH_KT6368_BOARD_H
#define WATCH_KT6368_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "watch_spp_transport.h"

#define WATCH_KT6368_DMA_BUFFER_SIZE 128U

typedef struct
{
    watch_spp_status_t transport;
    bool initialized;
    bool enable_active_low;
    bool recovery_pending;
    uint32_t uart_error_count;
    uint32_t tx_busy_count;
    uint32_t last_error_ms;
} watch_kt6368_board_status_t;

bool watch_kt6368_board_init(void);
void watch_kt6368_board_process(uint32_t now_ms);
bool watch_kt6368_board_set_enabled(bool enabled);
size_t watch_kt6368_board_read(uint8_t *data, size_t length);
size_t watch_kt6368_board_write(const uint8_t *data, size_t length);
bool watch_kt6368_board_read_status(watch_kt6368_board_status_t *status);

void watch_kt6368_board_on_rx_event(const UART_HandleTypeDef *huart, uint16_t size);
void watch_kt6368_board_on_tx_complete(const UART_HandleTypeDef *huart);
void watch_kt6368_board_on_error(const UART_HandleTypeDef *huart);
void watch_kt6368_board_dma_tx_irq(void);

#endif /* WATCH_KT6368_BOARD_H */
