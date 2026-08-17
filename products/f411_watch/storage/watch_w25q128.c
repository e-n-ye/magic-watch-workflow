#include "watch_w25q128.h"

#include <string.h>

static bool watch_w25q128_valid_device(const watch_w25q128_t *device)
{
    return device != NULL && device->initialized && device->bus.transfer != NULL
        && device->bus.now_ms != NULL && device->bus.delay_ms != NULL;
}

static bool watch_w25q128_range_valid(uint32_t address, size_t length)
{
    return address < WATCH_W25Q128_CAPACITY_BYTES
        && length <= (WATCH_W25Q128_CAPACITY_BYTES - address);
}

static watch_w25q128_result_t watch_w25q128_transfer(watch_w25q128_t *device, const uint8_t *tx,
                                                     uint8_t *rx, size_t length)
{
    if (!watch_w25q128_valid_device(device) || tx == NULL || rx == NULL || length == 0U
        || length > WATCH_W25Q128_MAX_TRANSFER_SIZE) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    return device->bus.transfer(device->bus.context, tx, rx, length)
        ? WATCH_W25Q128_RESULT_OK
        : WATCH_W25Q128_RESULT_BUS_ERROR;
}

bool watch_w25q128_init(watch_w25q128_t *device, const watch_w25q128_bus_t *bus)
{
    if (device == NULL || bus == NULL || bus->transfer == NULL || bus->now_ms == NULL
        || bus->delay_ms == NULL) {
        return false;
    }

    *device = (watch_w25q128_t) {
        .bus = *bus,
        .initialized = true,
    };
    return true;
}

watch_w25q128_result_t watch_w25q128_read_status(watch_w25q128_t *device, uint8_t *status)
{
    const uint8_t tx[] = { WATCH_W25Q128_COMMAND_READ_STATUS, 0U };
    uint8_t rx[sizeof(tx)] = { 0 };
    watch_w25q128_result_t result;

    if (status == NULL) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    result = watch_w25q128_transfer(device, tx, rx, sizeof(tx));
    if (result == WATCH_W25Q128_RESULT_OK) {
        *status = rx[1];
    }
    return result;
}

watch_w25q128_result_t watch_w25q128_wait_ready(watch_w25q128_t *device, uint32_t timeout_ms)
{
    uint32_t start_ms;
    uint8_t status;

    if (!watch_w25q128_valid_device(device)) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    start_ms = device->bus.now_ms(device->bus.context);
    for (;;) {
        watch_w25q128_result_t result = watch_w25q128_read_status(device, &status);
        if (result != WATCH_W25Q128_RESULT_OK) {
            return result;
        }
        if ((status & WATCH_W25Q128_STATUS_BUSY) == 0U) {
            return WATCH_W25Q128_RESULT_OK;
        }
        if ((uint32_t)(device->bus.now_ms(device->bus.context) - start_ms) >= timeout_ms) {
            return WATCH_W25Q128_RESULT_TIMEOUT;
        }
        device->bus.delay_ms(device->bus.context, 1U);
    }
}

watch_w25q128_result_t watch_w25q128_read_id(watch_w25q128_t *device, uint32_t *jedec_id)
{
    const uint8_t tx[] = { WATCH_W25Q128_COMMAND_READ_JEDEC_ID, 0U, 0U, 0U };
    uint8_t rx[sizeof(tx)] = { 0 };
    watch_w25q128_result_t result;

    if (jedec_id == NULL) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }

    result = watch_w25q128_wait_ready(device, 1000U);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    result = watch_w25q128_transfer(device, tx, rx, sizeof(tx));
    if (result == WATCH_W25Q128_RESULT_OK) {
        *jedec_id = ((uint32_t)rx[1] << 16U) | ((uint32_t)rx[2] << 8U) | rx[3];
    }
    return result;
}

watch_w25q128_result_t watch_w25q128_read(watch_w25q128_t *device, uint32_t address, uint8_t *data,
                                          size_t length, uint32_t timeout_ms)
{
    watch_w25q128_result_t result;

    if (data == NULL || length == 0U) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }
    if (!watch_w25q128_range_valid(address, length)) {
        return WATCH_W25Q128_RESULT_RANGE;
    }

    result = watch_w25q128_wait_ready(device, timeout_ms);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    while (length > 0U) {
        uint8_t tx[WATCH_W25Q128_MAX_TRANSFER_SIZE] = { 0 };
        uint8_t rx[WATCH_W25Q128_MAX_TRANSFER_SIZE] = { 0 };
        size_t chunk = length > WATCH_W25Q128_PAGE_SIZE ? WATCH_W25Q128_PAGE_SIZE : length;

        tx[0] = WATCH_W25Q128_COMMAND_READ_DATA;
        tx[1] = (uint8_t)(address >> 16U);
        tx[2] = (uint8_t)(address >> 8U);
        tx[3] = (uint8_t)address;
        result = watch_w25q128_transfer(device, tx, rx, chunk + 4U);
        if (result != WATCH_W25Q128_RESULT_OK) {
            return result;
        }
        memcpy(data, &rx[4], chunk);
        address += (uint32_t)chunk;
        data += chunk;
        length -= chunk;
    }

    return WATCH_W25Q128_RESULT_OK;
}

static watch_w25q128_result_t watch_w25q128_write_enable(watch_w25q128_t *device)
{
    const uint8_t tx[] = { WATCH_W25Q128_COMMAND_WRITE_ENABLE };
    uint8_t rx[sizeof(tx)] = { 0 };
    uint8_t status;
    watch_w25q128_result_t result = watch_w25q128_transfer(device, tx, rx, sizeof(tx));

    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }
    result = watch_w25q128_read_status(device, &status);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }
    return (status & WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH) != 0U
        ? WATCH_W25Q128_RESULT_OK
        : WATCH_W25Q128_RESULT_BUS_ERROR;
}

watch_w25q128_result_t watch_w25q128_page_program(watch_w25q128_t *device, uint32_t address,
                                                  const uint8_t *data, size_t length,
                                                  uint32_t timeout_ms)
{
    uint8_t tx[WATCH_W25Q128_MAX_TRANSFER_SIZE] = { 0 };
    uint8_t rx[WATCH_W25Q128_MAX_TRANSFER_SIZE] = { 0 };
    watch_w25q128_result_t result;

    if (data == NULL || length == 0U) {
        return WATCH_W25Q128_RESULT_INVALID_ARGUMENT;
    }
    if (!watch_w25q128_range_valid(address, length)) {
        return WATCH_W25Q128_RESULT_RANGE;
    }
    if (length > WATCH_W25Q128_PAGE_SIZE
        || (address & (WATCH_W25Q128_PAGE_SIZE - 1U)) + length > WATCH_W25Q128_PAGE_SIZE) {
        return WATCH_W25Q128_RESULT_ALIGNMENT;
    }

    result = watch_w25q128_wait_ready(device, timeout_ms);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    result = watch_w25q128_write_enable(device);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    tx[0] = WATCH_W25Q128_COMMAND_PAGE_PROGRAM;
    tx[1] = (uint8_t)(address >> 16U);
    tx[2] = (uint8_t)(address >> 8U);
    tx[3] = (uint8_t)address;
    memcpy(&tx[4], data, length);
    result = watch_w25q128_transfer(device, tx, rx, length + 4U);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    return watch_w25q128_wait_ready(device, timeout_ms);
}

watch_w25q128_result_t watch_w25q128_sector_erase(watch_w25q128_t *device, uint32_t address,
                                                  uint32_t timeout_ms)
{
    uint8_t tx[4] = { WATCH_W25Q128_COMMAND_SECTOR_ERASE, 0U, 0U, 0U };
    uint8_t rx[sizeof(tx)] = { 0 };
    watch_w25q128_result_t result;

    if (!watch_w25q128_range_valid(address, WATCH_W25Q128_SECTOR_SIZE)) {
        return WATCH_W25Q128_RESULT_RANGE;
    }
    if ((address & (WATCH_W25Q128_SECTOR_SIZE - 1U)) != 0U) {
        return WATCH_W25Q128_RESULT_ALIGNMENT;
    }

    result = watch_w25q128_wait_ready(device, timeout_ms);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    result = watch_w25q128_write_enable(device);
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    tx[1] = (uint8_t)(address >> 16U);
    tx[2] = (uint8_t)(address >> 8U);
    tx[3] = (uint8_t)address;
    result = watch_w25q128_transfer(device, tx, rx, sizeof(tx));
    if (result != WATCH_W25Q128_RESULT_OK) {
        return result;
    }

    return watch_w25q128_wait_ready(device, timeout_ms);
}

const char *watch_w25q128_result_name(watch_w25q128_result_t result)
{
    switch (result) {
    case WATCH_W25Q128_RESULT_OK:
        return "ok";
    case WATCH_W25Q128_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_W25Q128_RESULT_BUS_ERROR:
        return "bus";
    case WATCH_W25Q128_RESULT_TIMEOUT:
        return "timeout";
    case WATCH_W25Q128_RESULT_RANGE:
        return "range";
    case WATCH_W25Q128_RESULT_ALIGNMENT:
        return "alignment";
    case WATCH_W25Q128_RESULT_COUNT:
        return "invalid";
    }

    return "invalid";
}
