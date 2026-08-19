#include "watch_ota_install.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TEST_APP_SIZE (WATCH_OTA_INSTALL_SECTOR_SIZE * 2U)

typedef struct
{
    uint8_t app[TEST_APP_SIZE];
    uint8_t rollback[TEST_APP_SIZE];
    uint8_t candidate[TEST_APP_SIZE];
    uint32_t operation_count;
    int32_t fail_operation;
    uint32_t persist_count;
    int32_t fail_persist;
    watch_ota_metadata_record_t durable;
} fake_storage_t;

static uint8_t *region_data(fake_storage_t *storage, watch_ota_install_region_t region)
{
    switch (region) {
    case WATCH_OTA_INSTALL_REGION_APP:
        return storage->app;
    case WATCH_OTA_INSTALL_REGION_ROLLBACK:
        return storage->rollback;
    case WATCH_OTA_INSTALL_REGION_CANDIDATE:
        return storage->candidate;
    case WATCH_OTA_INSTALL_REGION_COUNT:
        break;
    }
    return NULL;
}

static bool operation_allowed(fake_storage_t *storage)
{
    ++storage->operation_count;
    if (storage->fail_operation > 0
        && storage->operation_count == (uint32_t)storage->fail_operation) {
        storage->fail_operation = -1;
        return false;
    }
    return true;
}

static bool fake_read(void *context, watch_ota_install_region_t region, uint32_t offset,
                      uint8_t *data, size_t length)
{
    fake_storage_t *storage = context;
    uint8_t *source = region_data(storage, region);

    if (source == NULL || (uint64_t)offset + length > TEST_APP_SIZE || !operation_allowed(storage)) {
        return false;
    }
    memcpy(data, source + offset, length);
    return true;
}

static bool fake_erase(void *context, watch_ota_install_region_t region, uint32_t offset,
                       size_t length)
{
    fake_storage_t *storage = context;
    uint8_t *destination = region_data(storage, region);

    if (destination == NULL || length != WATCH_OTA_INSTALL_SECTOR_SIZE
        || (uint64_t)offset + length > TEST_APP_SIZE || !operation_allowed(storage)) {
        return false;
    }
    memset(destination + offset, 0xFF, length);
    return true;
}

static bool fake_write(void *context, watch_ota_install_region_t region, uint32_t offset,
                       const uint8_t *data, size_t length)
{
    fake_storage_t *storage = context;
    uint8_t *destination = region_data(storage, region);

    if (destination == NULL || (uint64_t)offset + length > TEST_APP_SIZE
        || !operation_allowed(storage)) {
        return false;
    }
    memcpy(destination + offset, data, length);
    return true;
}

static bool fake_persist(void *context, const watch_ota_metadata_record_t *record)
{
    fake_storage_t *storage = context;

    ++storage->persist_count;
    if (storage->fail_persist > 0 && storage->persist_count == (uint32_t)storage->fail_persist) {
        storage->fail_persist = -1;
        return false;
    }
    storage->durable = *record;
    return true;
}

static void fake_init(fake_storage_t *storage, watch_ota_metadata_record_t *record)
{
    memset(storage, 0, sizeof(*storage));
    memset(storage->rollback, 0x00, sizeof(storage->rollback));
    for (size_t index = 0U; index < TEST_APP_SIZE; ++index) {
        storage->app[index] = (uint8_t)(index * 3U + 1U);
        storage->candidate[index] = (uint8_t)(0x80U + index * 5U);
    }
    *record = (watch_ota_metadata_record_t) {
        .sequence = 7U,
        .state = WATCH_OTA_METADATA_CANDIDATE_READY,
        .confirmed_counter = 2U,
        .candidate_counter = 3U,
        .candidate_version = 9U,
        .image_length = TEST_APP_SIZE,
        .progress = TEST_APP_SIZE,
    };
    storage->durable = *record;
    storage->fail_operation = -1;
    storage->fail_persist = -1;
}

static watch_ota_install_t make_install(fake_storage_t *storage)
{
    watch_ota_install_t install;
    const watch_ota_install_config_t config = {
        .read = fake_read,
        .erase = fake_erase,
        .write = fake_write,
        .persist = fake_persist,
        .context = storage,
        .app_size = TEST_APP_SIZE,
        .rollback_size = TEST_APP_SIZE,
        .candidate_size = TEST_APP_SIZE,
        .sector_size = WATCH_OTA_INSTALL_SECTOR_SIZE,
    };

    assert(watch_ota_install_init(&install, &config));
    return install;
}

static void finish_install(watch_ota_install_t *install, watch_ota_metadata_record_t *record)
{
    unsigned int guard = 0U;

    while (record->state != WATCH_OTA_METADATA_TRIAL) {
        assert(guard++ < 100U);
        assert(watch_ota_install_step(install, record) == WATCH_OTA_INSTALL_RESULT_OK);
    }
    assert(watch_ota_install_step(install, record) == WATCH_OTA_INSTALL_RESULT_COMPLETE);
}

static void test_successful_install(void)
{
    fake_storage_t storage;
    watch_ota_metadata_record_t record;
    watch_ota_install_t install;

    fake_init(&storage, &record);
    install = make_install(&storage);
    assert(watch_ota_install_start(&install, &record) == WATCH_OTA_INSTALL_RESULT_OK);
    assert(record.state == WATCH_OTA_METADATA_BACKING_UP && record.progress == 0U);
    finish_install(&install, &record);
    assert(record.state == WATCH_OTA_METADATA_TRIAL && record.progress == TEST_APP_SIZE);
    assert(record.trial_count == 0U);
    assert(memcmp(storage.rollback, storage.app, sizeof(storage.app)) != 0);
    for (size_t index = 0U; index < TEST_APP_SIZE; ++index) {
        assert(storage.rollback[index] == (uint8_t)(index * 3U + 1U));
        assert(storage.app[index] == (uint8_t)(0x80U + index * 5U));
    }
    assert(storage.durable.state == WATCH_OTA_METADATA_TRIAL);
}

static void test_storage_fault_recovery(int32_t failure_number)
{
    fake_storage_t storage;
    watch_ota_metadata_record_t record;
    watch_ota_install_t install;
    watch_ota_install_result_t result;

    fake_init(&storage, &record);
    install = make_install(&storage);
    assert(watch_ota_install_start(&install, &record) == WATCH_OTA_INSTALL_RESULT_OK);
    storage.fail_operation = failure_number;
    do {
        result = watch_ota_install_step(&install, &record);
        if (result == WATCH_OTA_INSTALL_RESULT_IO) {
            assert(record.state == storage.durable.state);
            break;
        }
        assert(result == WATCH_OTA_INSTALL_RESULT_OK);
    } while (record.state != WATCH_OTA_METADATA_TRIAL);
    storage.fail_operation = -1;
    finish_install(&install, &record);
    assert(record.state == WATCH_OTA_METADATA_TRIAL);
    assert(storage.durable.state == WATCH_OTA_METADATA_TRIAL);
}

static void test_persist_fault_recovery(void)
{
    fake_storage_t storage;
    watch_ota_metadata_record_t record;
    watch_ota_install_t install;

    fake_init(&storage, &record);
    install = make_install(&storage);
    assert(watch_ota_install_start(&install, &record) == WATCH_OTA_INSTALL_RESULT_OK);
    storage.fail_persist = 2;
    assert(watch_ota_install_step(&install, &record) == WATCH_OTA_INSTALL_RESULT_PERSIST);
    assert(record.state == WATCH_OTA_METADATA_BACKING_UP && record.progress == 0U);
    assert(storage.durable.state == WATCH_OTA_METADATA_BACKING_UP
           && storage.durable.progress == 0U);
    storage.fail_persist = -1;
    finish_install(&install, &record);
}

static void test_invalid_state(void)
{
    fake_storage_t storage;
    watch_ota_metadata_record_t record;
    watch_ota_install_t install;

    fake_init(&storage, &record);
    install = make_install(&storage);
    record.state = WATCH_OTA_METADATA_DOWNLOADING;
    assert(watch_ota_install_start(&install, &record) == WATCH_OTA_INSTALL_RESULT_INVALID_STATE);
    assert(watch_ota_install_step(&install, &record) == WATCH_OTA_INSTALL_RESULT_INVALID_STATE);
    assert(strcmp(watch_ota_install_result_name(WATCH_OTA_INSTALL_RESULT_PERSIST), "persist") == 0);
}

int main(void)
{
    test_successful_install();
    test_storage_fault_recovery(1);
    test_storage_fault_recovery(3);
    test_storage_fault_recovery(9);
    test_storage_fault_recovery(11);
    test_persist_fault_recovery();
    test_invalid_state();
    return 0;
}
