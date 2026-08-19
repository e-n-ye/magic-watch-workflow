#include "watch_ota_install.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define F411_APP_SIZE 0x70000U
#define F411_SECTOR4_SIZE 0x10000U
#define F411_SECTOR5_SIZE 0x20000U
#define F411_SECTOR6_SIZE 0x20000U
#define F411_SECTOR7_SIZE 0x20000U

typedef struct
{
    uint8_t app[F411_APP_SIZE];
    uint8_t rollback[F411_APP_SIZE];
    uint8_t candidate[F411_APP_SIZE];
    unsigned int app_erase_count;
    unsigned int rollback_erase_count;
} f411_storage_t;

static uint8_t *region_data(f411_storage_t *storage, watch_ota_install_region_t region)
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

static bool read_region(void *context, watch_ota_install_region_t region, uint32_t offset,
                        uint8_t *data, size_t length)
{
    f411_storage_t *storage = context;
    uint8_t *source = region_data(storage, region);

    if (source == NULL || offset > F411_APP_SIZE || length > F411_APP_SIZE - offset) {
        return false;
    }
    memcpy(data, source + offset, length);
    return true;
}

static bool erase_region(void *context, watch_ota_install_region_t region, uint32_t offset,
                         size_t length)
{
    f411_storage_t *storage = context;
    uint8_t *destination = region_data(storage, region);

    if (destination == NULL || length != WATCH_OTA_INSTALL_SECTOR_SIZE
        || offset % WATCH_OTA_INSTALL_SECTOR_SIZE != 0U
        || offset > F411_APP_SIZE - WATCH_OTA_INSTALL_SECTOR_SIZE) {
        return false;
    }

    if (region == WATCH_OTA_INSTALL_REGION_APP) {
        if (offset != 0U && offset != F411_SECTOR4_SIZE
            && offset != F411_SECTOR4_SIZE + F411_SECTOR5_SIZE
            && offset != F411_SECTOR4_SIZE + F411_SECTOR5_SIZE + F411_SECTOR6_SIZE) {
            return true;
        }
        size_t sector_size = offset == 0U ? F411_SECTOR4_SIZE
            : offset < F411_SECTOR4_SIZE + F411_SECTOR5_SIZE ? F411_SECTOR5_SIZE
            : offset < F411_SECTOR4_SIZE + F411_SECTOR5_SIZE + F411_SECTOR6_SIZE
                ? F411_SECTOR6_SIZE
                : F411_SECTOR7_SIZE;
        memset(destination + offset, 0xFF, sector_size);
        ++storage->app_erase_count;
    } else if (region == WATCH_OTA_INSTALL_REGION_ROLLBACK) {
        memset(destination + offset, 0xFF, WATCH_OTA_INSTALL_SECTOR_SIZE);
        ++storage->rollback_erase_count;
    } else {
        memset(destination + offset, 0xFF, WATCH_OTA_INSTALL_SECTOR_SIZE);
    }
    return true;
}

static bool write_region(void *context, watch_ota_install_region_t region, uint32_t offset,
                         const uint8_t *data, size_t length)
{
    f411_storage_t *storage = context;
    uint8_t *destination = region_data(storage, region);

    if (destination == NULL || offset > F411_APP_SIZE || length > F411_APP_SIZE - offset) {
        return false;
    }
    memcpy(destination + offset, data, length);
    return true;
}

static bool persist_record(void *context, const watch_ota_metadata_record_t *record)
{
    (void)context;
    (void)record;
    return true;
}

int main(void)
{
    f411_storage_t storage;
    watch_ota_install_t install;
    watch_ota_metadata_record_t record = {
        .sequence = 1U,
        .state = WATCH_OTA_METADATA_CANDIDATE_READY,
        .confirmed_counter = 23U,
        .candidate_counter = 24U,
        .candidate_version = 24U,
        .image_length = 358600U,
        .progress = F411_APP_SIZE,
    };
    const watch_ota_install_config_t config = {
        .read = read_region,
        .erase = erase_region,
        .write = write_region,
        .persist = persist_record,
        .context = &storage,
        .app_size = F411_APP_SIZE,
        .rollback_size = F411_APP_SIZE,
        .candidate_size = F411_APP_SIZE,
        .sector_size = WATCH_OTA_INSTALL_SECTOR_SIZE,
    };

    memset(&storage, 0, sizeof(storage));
    for (size_t index = 0U; index < F411_APP_SIZE; ++index) {
        storage.app[index] = (uint8_t)(index * 5U + 1U);
        storage.candidate[index] = (uint8_t)(index * 7U + 3U);
    }
    assert(watch_ota_install_init(&install, &config));
    assert(watch_ota_install_start(&install, &record) == WATCH_OTA_INSTALL_RESULT_OK);
    while (record.state != WATCH_OTA_METADATA_TRIAL) {
        assert(watch_ota_install_step(&install, &record) == WATCH_OTA_INSTALL_RESULT_OK);
    }
    assert(watch_ota_install_step(&install, &record) == WATCH_OTA_INSTALL_RESULT_COMPLETE);
    assert(storage.rollback_erase_count == F411_APP_SIZE / WATCH_OTA_INSTALL_SECTOR_SIZE);
    assert(storage.app_erase_count == 4U);
    for (size_t index = 0U; index < F411_APP_SIZE; ++index) {
        assert(storage.rollback[index] == (uint8_t)(index * 5U + 1U));
        assert(storage.app[index] == storage.candidate[index]);
    }
    return 0;
}
