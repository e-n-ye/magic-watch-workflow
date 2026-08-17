#ifndef WATCH_W25Q128_H
#define WATCH_W25Q128_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATCH_W25Q128_CAPACITY_BYTES 0x01000000UL
#define WATCH_W25Q128_PAGE_SIZE 256U
#define WATCH_W25Q128_SECTOR_SIZE 4096U
#define WATCH_W25Q128_MAX_TRANSFER_SIZE (WATCH_W25Q128_PAGE_SIZE + 4U)

#define WATCH_W25Q128_COMMAND_READ_STATUS 0x05U
#define WATCH_W25Q128_COMMAND_WRITE_ENABLE 0x06U
#define WATCH_W25Q128_COMMAND_READ_DATA 0x03U
#define WATCH_W25Q128_COMMAND_PAGE_PROGRAM 0x02U
#define WATCH_W25Q128_COMMAND_SECTOR_ERASE 0x20U
#define WATCH_W25Q128_COMMAND_READ_JEDEC_ID 0x9FU

#define WATCH_W25Q128_STATUS_BUSY 0x01U
#define WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH 0x02U

typedef enum {
    WATCH_W25Q128_RESULT_OK = 0,
    WATCH_W25Q128_RESULT_INVALID_ARGUMENT,
    WATCH_W25Q128_RESULT_BUS_ERROR,
    WATCH_W25Q128_RESULT_TIMEOUT,
    WATCH_W25Q128_RESULT_RANGE,
    WATCH_W25Q128_RESULT_ALIGNMENT,
    WATCH_W25Q128_RESULT_COUNT
} watch_w25q128_result_t;

typedef bool (*watch_w25q128_transfer_fn)(void *context, const uint8_t *tx, uint8_t *rx,
                                          size_t length);
typedef uint32_t (*watch_w25q128_now_ms_fn)(void *context);
typedef void (*watch_w25q128_delay_ms_fn)(void *context, uint32_t delay_ms);

typedef struct
{
    watch_w25q128_transfer_fn transfer;
    watch_w25q128_now_ms_fn now_ms;
    watch_w25q128_delay_ms_fn delay_ms;
    void *context;
} watch_w25q128_bus_t;

typedef struct
{
    watch_w25q128_bus_t bus;
    bool initialized;
} watch_w25q128_t;

bool watch_w25q128_init(watch_w25q128_t *device, const watch_w25q128_bus_t *bus);
watch_w25q128_result_t watch_w25q128_read_status(watch_w25q128_t *device, uint8_t *status);
watch_w25q128_result_t watch_w25q128_wait_ready(watch_w25q128_t *device, uint32_t timeout_ms);
watch_w25q128_result_t watch_w25q128_read_id(watch_w25q128_t *device, uint32_t *jedec_id);
watch_w25q128_result_t watch_w25q128_read(watch_w25q128_t *device, uint32_t address, uint8_t *data,
                                          size_t length, uint32_t timeout_ms);
watch_w25q128_result_t watch_w25q128_page_program(watch_w25q128_t *device, uint32_t address,
                                                  const uint8_t *data, size_t length,
                                                  uint32_t timeout_ms);
watch_w25q128_result_t watch_w25q128_sector_erase(watch_w25q128_t *device, uint32_t address,
                                                  uint32_t timeout_ms);
const char *watch_w25q128_result_name(watch_w25q128_result_t result);

#endif /* WATCH_W25Q128_H */
