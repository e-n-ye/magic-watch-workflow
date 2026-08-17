#ifndef WATCH_W25Q128_BOARD_H
#define WATCH_W25Q128_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "watch_w25q128.h"

#define WATCH_W25Q128_BOARD_DEFAULT_TIMEOUT_MS 1000U
#define WATCH_W25Q128_BOARD_TEST_ADDRESS 0x00F000U

bool watch_w25q128_board_init(void);
watch_w25q128_result_t watch_w25q128_board_read_id(uint32_t *jedec_id);
watch_w25q128_result_t watch_w25q128_board_wait_ready(uint32_t timeout_ms);
watch_w25q128_result_t watch_w25q128_board_read(uint32_t address, uint8_t *data, size_t length);
watch_w25q128_result_t watch_w25q128_board_page_program(uint32_t address, const uint8_t *data,
                                                        size_t length);
watch_w25q128_result_t watch_w25q128_board_sector_erase(uint32_t address);

#endif /* WATCH_W25Q128_BOARD_H */
