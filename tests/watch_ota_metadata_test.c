#include "watch_ota_metadata.h"
#include "watch_ota_trial.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FAKE_FLASH_SIZE (WATCH_OTA_METADATA_SLOT_COUNT * WATCH_OTA_METADATA_SLOT_SIZE)

typedef struct
{
    uint8_t memory[FAKE_FLASH_SIZE];
    uint8_t status;
    uint32_t now_ms;
    bool fail_transfer;
    bool fail_erase;
    bool fail_program;
} fake_flash_t;

static void fake_flash_init(fake_flash_t *flash)
{
    memset(flash, 0, sizeof(*flash));
    memset(flash->memory, 0xFF, sizeof(flash->memory));
}

static bool fake_flash_transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
    fake_flash_t *flash = context;
    uint32_t address;

    assert(flash != NULL && tx != NULL && rx != NULL && length > 0U);
    memset(rx, 0, length);
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
    case WATCH_W25Q128_COMMAND_READ_DATA:
        assert(length >= 4U);
        address = ((uint32_t)tx[1] << 16U) | ((uint32_t)tx[2] << 8U) | tx[3];
        assert(address + length - 4U <= sizeof(flash->memory));
        memcpy(&rx[4], &flash->memory[address], length - 4U);
        return true;
    case WATCH_W25Q128_COMMAND_PAGE_PROGRAM:
        assert(length >= 5U);
        assert((flash->status & WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH) != 0U);
        if (flash->fail_program) {
            return false;
        }
        address = ((uint32_t)tx[1] << 16U) | ((uint32_t)tx[2] << 8U) | tx[3];
        assert(address + length - 4U <= sizeof(flash->memory));
        for (size_t index = 0U; index < length - 4U; ++index) {
            flash->memory[address + index] &= tx[4U + index];
        }
        flash->status = WATCH_W25Q128_STATUS_BUSY;
        return true;
    case WATCH_W25Q128_COMMAND_SECTOR_ERASE:
        assert(length == 4U);
        assert((flash->status & WATCH_W25Q128_STATUS_WRITE_ENABLE_LATCH) != 0U);
        if (flash->fail_erase) {
            return false;
        }
        address = ((uint32_t)tx[1] << 16U) | ((uint32_t)tx[2] << 8U) | tx[3];
        assert(address + WATCH_W25Q128_SECTOR_SIZE <= sizeof(flash->memory));
        memset(&flash->memory[address], 0xFF, WATCH_W25Q128_SECTOR_SIZE);
        flash->status = WATCH_W25Q128_STATUS_BUSY;
        return true;
    default:
        return false;
    }
}

static uint32_t fake_flash_now(void *context)
{
    return ((fake_flash_t *)context)->now_ms;
}

static void fake_flash_delay(void *context, uint32_t delay_ms)
{
    fake_flash_t *flash = context;
    flash->now_ms += delay_ms;
    flash->status &= (uint8_t)~WATCH_W25Q128_STATUS_BUSY;
}

static bool setup(fake_flash_t *flash, watch_w25q128_t *device, watch_ota_metadata_t *metadata)
{
    const watch_w25q128_bus_t bus = {
        .transfer = fake_flash_transfer,
        .now_ms = fake_flash_now,
        .delay_ms = fake_flash_delay,
        .context = flash,
    };

    fake_flash_init(flash);
    return watch_w25q128_init(device, &bus) && watch_ota_metadata_init(metadata, device);
}

static watch_ota_metadata_record_t make_record(uint32_t counter, uint8_t digest_value)
{
    watch_ota_metadata_record_t record = {
        .state = WATCH_OTA_METADATA_CANDIDATE_READY,
        .candidate_counter = counter,
        .candidate_version = counter,
        .image_length = 1024U,
        .progress = 1024U,
    };

    for (size_t index = 0U; index < WATCH_OTA_METADATA_DIGEST_SIZE; ++index) {
        record.candidate_digest[index] = digest_value;
    }
    return record;
}

static void test_empty_and_round_trip(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    watch_ota_metadata_t metadata;
    watch_ota_metadata_record_t input = make_record(1U, 0x11U);
    watch_ota_metadata_record_t output;

    assert(setup(&flash, &device, &metadata));
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_EMPTY);
    input.sequence = 99U;
    assert(watch_ota_metadata_commit(&metadata, &input) == WATCH_OTA_METADATA_RESULT_OK);
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.sequence == 1U && output.state == input.state);
    assert(output.candidate_counter == 1U && output.image_length == 1024U);
    assert(memcmp(output.candidate_digest, input.candidate_digest,
                  WATCH_OTA_METADATA_DIGEST_SIZE) == 0);
    assert(flash.memory[0] == 'M' && flash.memory[4] == WATCH_OTA_METADATA_FORMAT_VERSION);
}

static void test_sequence_and_crc_fallback(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    watch_ota_metadata_t metadata;
    watch_ota_metadata_record_t record = make_record(1U, 0x22U);
    watch_ota_metadata_record_t output;

    assert(setup(&flash, &device, &metadata));
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    record = make_record(2U, 0x33U);
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.sequence == 2U && output.candidate_counter == 2U);

    flash.memory[WATCH_OTA_METADATA_SLOT_OFFSET(1U) + 10U] ^= 0x01U;
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.sequence == 1U && output.candidate_counter == 1U);
    flash.memory[WATCH_OTA_METADATA_SLOT_OFFSET(0U) + 10U] ^= 0x01U;
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_CORRUPT);
}

static void test_security_policy(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    watch_ota_metadata_t metadata;
    watch_ota_metadata_record_t record;

    assert(setup(&flash, &device, &metadata));
    record = make_record(5U, 0x44U);
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    record.confirmed_counter = 1U;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    record = make_record(5U, 0x44U);
    assert(watch_ota_metadata_commit(&metadata, &record)
           == WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED);
    record = make_record(4U, 0x44U);
    assert(watch_ota_metadata_commit(&metadata, &record)
           == WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED);
    record = make_record(5U, 0x55U);
    record.confirmed_counter = 1U;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_CONFLICT);
    record = make_record(5U, 0x44U);
    record.confirmed_counter = 1U;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    record = make_record(6U, 0x66U);
    record.confirmed_counter = 1U;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    record.image_length = WATCH_W25_CANDIDATE_SIZE + 1U;
    assert(watch_ota_metadata_commit(&metadata, &record)
           == WATCH_OTA_METADATA_RESULT_INVALID_RECORD);
}

static void test_install_progress_window(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    watch_ota_metadata_t metadata;
    watch_ota_metadata_record_t record = make_record(1U, 0x66U);
    watch_ota_metadata_record_t output;

    assert(setup(&flash, &device, &metadata));
    record.state = WATCH_OTA_METADATA_BACKING_UP;
    record.progress = WATCH_W25_CANDIDATE_SIZE;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.state == WATCH_OTA_METADATA_BACKING_UP
           && output.progress == WATCH_W25_CANDIDATE_SIZE);

    record.state = WATCH_OTA_METADATA_CANDIDATE_READY;
    assert(watch_ota_metadata_commit(&metadata, &record)
           == WATCH_OTA_METADATA_RESULT_INVALID_RECORD);
}

static void test_write_failure_and_invalid_copies(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    watch_ota_metadata_t metadata;
    watch_ota_metadata_record_t record = make_record(1U, 0x77U);
    watch_ota_metadata_record_t output;

    assert(setup(&flash, &device, &metadata));
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    record = make_record(2U, 0x88U);
    flash.fail_erase = true;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_IO);
    flash.fail_erase = false;
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.sequence == 1U && output.candidate_counter == 1U);

    flash.fail_program = true;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_IO);
    flash.fail_program = false;
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.sequence == 1U && output.candidate_counter == 1U);

    flash.memory[WATCH_OTA_METADATA_SLOT_OFFSET(0U)] = 0U;
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_CORRUPT);
}

static void test_trial_confirmation_commit(void)
{
    fake_flash_t flash;
    watch_w25q128_t device;
    watch_ota_metadata_t metadata;
    watch_ota_metadata_record_t record = make_record(25U, 0x25U);
    watch_ota_metadata_record_t output;

    assert(setup(&flash, &device, &metadata));
    record.state = WATCH_OTA_METADATA_TRIAL;
    record.progress = WATCH_W25_CANDIDATE_SIZE;
    assert(watch_ota_metadata_commit(&metadata, &record) == WATCH_OTA_METADATA_RESULT_OK);
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(watch_ota_trial_confirm(&output) == WATCH_OTA_TRIAL_RESULT_OK);
    assert(watch_ota_metadata_commit(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(watch_ota_metadata_load(&metadata, &output) == WATCH_OTA_METADATA_RESULT_OK);
    assert(output.state == WATCH_OTA_METADATA_CONFIRMED);
    assert(output.confirmed_counter == 25U);
}

int main(void)
{
    test_empty_and_round_trip();
    test_sequence_and_crc_fallback();
    test_security_policy();
    test_install_progress_window();
    test_write_failure_and_invalid_copies();
    test_trial_confirmation_commit();
    assert(strcmp(watch_ota_metadata_state_name(WATCH_OTA_METADATA_TRIAL), "trial") == 0);
    assert(strcmp(watch_ota_metadata_result_name(WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED),
                  "security") == 0);
    return 0;
}
