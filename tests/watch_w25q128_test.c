#include "watch_w25q128.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FAKE_FLASH_SIZE 8192U

typedef struct
{
    uint8_t memory[FAKE_FLASH_SIZE];
    uint8_t status;
    uint32_t now_ms;
    uint32_t transfer_count;
    bool hold_busy;
    bool fail_transfer;
} fake_flash_t;

static void fake_flash_init(fake_flash_t *flash)
{
    memset(flash, 0, sizeof(*flash));
    memset(flash->memory, 0xFF, sizeof(flash->memory));
}

static bool fake_flash_transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
    fake_flash_t *flash = (fake_flash_t *)context;
    uint32_t address;

    assert(flash != NULL);
    assert(tx != NULL);
    assert(rx != NULL);
    assert(length > 0U);
    memset(rx, 0, length);
    flash->transfer_count++;
    if (flash->fail_transfer) {
        return false;
    }

    switch (tx[0]) {
    case WATCH_W25Q128_COMMAND_READ_STATUS:
        assert(length == 2U);
        rx[1] = flash->status;
        return true;
    case WATCH_W25Q128_COMMAND_WRITE_ENABLE:
        assert(length == 1U);
        flash->status |= WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH;
        return true;
    case WATCH_W25Q128_COMMAND_READ_JEDEC_ID:
        assert(length == 4U);
        rx[1] = 0xEFU;
        rx[2] = 0x40U;
        rx[3] = 0x18U;
        return true;
    case WATCH_W25Q128_COMMAND_READ_DATA:
        assert(length >= 4U);
        address = ((uint32_t)tx[1] << 16U) | ((uint32_t)tx[2] << 8U) | tx[3];
        assert(address + (length - 4U) <= FAKE_FLASH_SIZE);
        memcpy(&rx[4], &flash->memory[address], length - 4U);
        return true;
    case WATCH_W25Q128_COMMAND_PAGE_PROGRAM:
        assert(length >= 5U);
        assert((flash->status & WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH) != 0U);
        address = ((uint32_t)tx[1] << 16U) | ((uint32_t)tx[2] << 8U) | tx[3];
        assert(address + (length - 4U) <= FAKE_FLASH_SIZE);
        for (size_t index = 0U; index < length - 4U; ++index) {
            flash->memory[address + index] &= tx[4U + index];
        }
        flash->status = WATCH_W25Q128_STATUS_BUSY;
        return true;
    case WATCH_W25Q128_COMMAND_SECTOR_ERASE:
        assert(length == 4U);
        assert((flash->status & WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH) != 0U);
        address = ((uint32_t)tx[1] << 16U) | ((uint32_t)tx[2] << 8U) | tx[3];
        assert(address + WATCH_W25Q128_SECTOR_SIZE <= FAKE_FLASH_SIZE);
        memset(&flash->memory[address], 0xFF, WATCH_W25Q128_SECTOR_SIZE);
        flash->status = WATCH_W25Q128_STATUS_BUSY;
        return true;
    default:
        return false;
    }
}

static uint32_t fake_flash_now(void *context)
{
    fake_flash_t *flash = (fake_flash_t *)context;
    return flash->now_ms;
}

static void fake_flash_delay(void *context, uint32_t delay_ms)
{
    fake_flash_t *flash = (fake_flash_t *)context;
    flash->now_ms += delay_ms;
    if (!flash->hold_busy) {
        flash->status &= (uint8_t)~WATCH_W25Q128_STATUS_BUSY;
    }
}

static bool fake_flash_device_init(fake_flash_t *flash, watch_w25q128_t *device)
{
    watch_w25q128_bus_t bus = {
        .transfer = fake_flash_transfer,
        .now_ms = fake_flash_now,
        .delay_ms = fake_flash_delay,
        .context = flash,
    };

    return watch_w25q128_init(device, &bus);
}

static void test_read_id_and_ready(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    uint32_t id = 0U;

    fake_flash_init(&flash);
    assert(fake_flash_device_init(&flash, &device));
    assert(watch_w25q128_read_id(&device, &id) == WATCH_W25Q128_RESULT_OK);
    assert(id == 0xEF4018UL);
    assert(watch_w25q128_wait_ready(&device, 2U) == WATCH_W25Q128_RESULT_OK);
}

static void test_page_program_and_read(void)
{
    const uint8_t expected[] = { 0x00U, 0x11U, 0x22U, 0x33U };
    fake_flash_t flash;
    watch_w25q128_t device;
    uint8_t actual[sizeof(expected)] = { 0 };

    fake_flash_init(&flash);
    assert(fake_flash_device_init(&flash, &device));
    assert(watch_w25q128_sector_erase(&device, 0U, 10U) == WATCH_W25Q128_RESULT_OK);
    assert(watch_w25q128_page_program(&device, 12U, expected, sizeof(expected), 10U)
           == WATCH_W25Q128_RESULT_OK);
    assert(watch_w25q128_read(&device, 12U, actual, sizeof(actual), 10U)
           == WATCH_W25Q128_RESULT_OK);
    assert(memcmp(expected, actual, sizeof(expected)) == 0);
}

static void test_read_chunks_and_boundaries(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    uint8_t data[300] = { 0 };

    fake_flash_init(&flash);
    for (size_t index = 0U; index < sizeof(data); ++index) {
        flash.memory[200U + index] = (uint8_t)index;
    }
    assert(fake_flash_device_init(&flash, &device));
    assert(watch_w25q128_read(&device, 200U, data, sizeof(data), 10U) == WATCH_W25Q128_RESULT_OK);
    for (size_t index = 0U; index < sizeof(data); ++index) {
        assert(data[index] == (uint8_t)index);
    }
    assert(watch_w25q128_read(&device, WATCH_W25Q128_CAPACITY_BYTES - 2U, data, 3U, 10U)
           == WATCH_W25Q128_RESULT_RANGE);
}

static void test_invalid_alignment_and_timeout(void)
{
    const uint8_t byte = 0xA5U;
    fake_flash_t flash;
    watch_w25q128_t device;

    fake_flash_init(&flash);
    assert(fake_flash_device_init(&flash, &device));
    assert(watch_w25q128_page_program(&device, 255U, &byte, 2U, 10U)
           == WATCH_W25Q128_RESULT_ALIGNMENT);
    assert(watch_w25q128_sector_erase(&device, 1U, 10U) == WATCH_W25Q128_RESULT_ALIGNMENT);
    assert(watch_w25q128_sector_erase(&device, WATCH_W25Q128_CAPACITY_BYTES, 10U)
           == WATCH_W25Q128_RESULT_RANGE);

    flash.status = WATCH_W25Q128_STATUS_BUSY;
    flash.hold_busy = true;
    assert(watch_w25q128_wait_ready(&device, 2U) == WATCH_W25Q128_RESULT_TIMEOUT);
    flash.fail_transfer = true;
    assert(watch_w25q128_wait_ready(&device, 2U) == WATCH_W25Q128_RESULT_BUS_ERROR);
}

static void test_invalid_arguments(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;

    fake_flash_init(&flash);
    assert(!watch_w25q128_init(NULL, NULL));
    assert(fake_flash_device_init(&flash, &device));
    assert(watch_w25q128_read_id(&device, NULL) == WATCH_W25Q128_RESULT_INVALID_ARGUMENT);
    assert(watch_w25q128_read(&device, 0U, NULL, 1U, 1U) == WATCH_W25Q128_RESULT_INVALID_ARGUMENT);
}

int main(void)
{
    test_read_id_and_ready();
    test_page_program_and_read();
    test_read_chunks_and_boundaries();
    test_invalid_alignment_and_timeout();
    test_invalid_arguments();
    assert(strcmp(watch_w25q128_result_name(WATCH_W25Q128_RESULT_TIMEOUT), "timeout") == 0);
    return 0;
}
