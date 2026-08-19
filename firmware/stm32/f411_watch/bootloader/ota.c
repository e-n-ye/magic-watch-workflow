#include "ota.h"

#include <string.h>

#include "manifest.h"
#include "storage.h"

#include "watch_ota_metadata.h"
#include "watch_ota_package.h"

#define BOOTLOADER_OTA_ERROR_VERIFY 1U
#define BOOTLOADER_OTA_ERROR_INSTALL 2U
#define BOOTLOADER_OTA_PERSIST_STRIDE 16U

static watch_ota_metadata_t *s_metadata;
static watch_ota_metadata_record_t s_last_persisted;
static bool s_have_last_persisted;
static uint32_t s_persist_count;

static bool persist_install_record(void *context, const watch_ota_metadata_record_t *record)
{
    bool force;

    (void)context;
    if (record == NULL || f411_bootloader_w25() == NULL) {
        return false;
    }

    force = !s_have_last_persisted || record->state != s_last_persisted.state
        || record->progress == 0U || record->progress == F411_BOOTLOADER_APP_SIZE
        || (++s_persist_count % BOOTLOADER_OTA_PERSIST_STRIDE) == 0U;
    if (force && watch_ota_metadata_commit(s_metadata, record) != WATCH_OTA_METADATA_RESULT_OK) {
        return false;
    }
    s_last_persisted = *record;
    s_have_last_persisted = true;
    return true;
}

static bool mark_error(const watch_ota_metadata_record_t *record, uint32_t error_code)
{
    watch_ota_metadata_record_t next;

    if (record == NULL || s_metadata == NULL) {
        return false;
    }
    next = *record;
    next.state = WATCH_OTA_METADATA_ERROR;
    next.error_code = error_code;
    return watch_ota_metadata_commit(s_metadata, &next) == WATCH_OTA_METADATA_RESULT_OK;
}

static bool candidate_matches_record(const watch_ota_metadata_record_t *record,
                                     const watch_ota_package_info_t *info)
{
    return record != NULL && info != NULL && info->security_counter == record->candidate_counter
        && info->firmware_version == record->candidate_version
        && info->image_length == record->image_length
        && memcmp(info->digest, record->candidate_digest, sizeof(info->digest)) == 0;
}

static bool verify_candidate(const watch_ota_metadata_record_t *record)
{
    const watch_ota_package_reader_t reader = {
        .read = f411_bootloader_candidate_read,
        .context = NULL,
        .size = WATCH_OTA_PACKAGE_SIZE,
    };
    watch_ota_package_info_t info;
    uint32_t minimum_counter;
    watch_ota_package_result_t result;

    minimum_counter = record->confirmed_counter;
    result = watch_ota_package_verify(&reader, WATCH_OTA_PACKAGE_BOARD_ID, minimum_counter,
                                      bootloader_public_key(), &info);
    return result == WATCH_OTA_PACKAGE_RESULT_OK && candidate_matches_record(record, &info);
}

int f411_bootloader_process_ota(void)
{
    watch_ota_metadata_record_t record;
    watch_ota_metadata_result_t metadata_result;
    watch_ota_install_t install;
    const watch_ota_install_config_t config = {
        .read = f411_bootloader_storage_read,
        .erase = f411_bootloader_storage_erase,
        .write = f411_bootloader_storage_write,
        .persist = persist_install_record,
        .context = &record,
        .app_size = F411_BOOTLOADER_APP_SIZE,
        .rollback_size = WATCH_W25_ROLLBACK_SIZE,
        .candidate_size = WATCH_W25_CANDIDATE_SIZE,
        .sector_size = WATCH_OTA_INSTALL_SECTOR_SIZE,
    };

    if (!f411_bootloader_storage_init()) {
        return 0;
    }
    f411_bootloader_watchdog_extend();
    s_metadata = f411_bootloader_metadata();
    metadata_result = watch_ota_metadata_load(s_metadata, &record);
    if (metadata_result == WATCH_OTA_METADATA_RESULT_EMPTY) {
        return 0;
    }
    if (metadata_result != WATCH_OTA_METADATA_RESULT_OK) {
        return 0;
    }
    if (record.state != WATCH_OTA_METADATA_CANDIDATE_READY
        && record.state != WATCH_OTA_METADATA_BACKING_UP
        && record.state != WATCH_OTA_METADATA_INSTALLING) {
        return 0;
    }
    if (!verify_candidate(&record)) {
        (void)mark_error(&record, BOOTLOADER_OTA_ERROR_VERIFY);
        return -1;
    }
    if (record.state == WATCH_OTA_METADATA_BACKING_UP) {
        if (!f411_bootloader_storage_prepare_resume(WATCH_OTA_INSTALL_REGION_ROLLBACK,
                                                    &record.progress)) {
            (void)mark_error(&record, BOOTLOADER_OTA_ERROR_INSTALL);
            return -1;
        }
    } else if (record.state == WATCH_OTA_METADATA_INSTALLING) {
        if (!f411_bootloader_storage_prepare_resume(WATCH_OTA_INSTALL_REGION_APP,
                                                    &record.progress)) {
            (void)mark_error(&record, BOOTLOADER_OTA_ERROR_INSTALL);
            return -1;
        }
    }
    if (!watch_ota_install_init(&install, &config)) {
        (void)mark_error(&record, BOOTLOADER_OTA_ERROR_INSTALL);
        return -1;
    }
    if (record.state == WATCH_OTA_METADATA_CANDIDATE_READY
        && watch_ota_install_start(&install, &record) != WATCH_OTA_INSTALL_RESULT_OK) {
        (void)mark_error(&record, BOOTLOADER_OTA_ERROR_INSTALL);
        return -1;
    }

    while (record.state == WATCH_OTA_METADATA_BACKING_UP
           || record.state == WATCH_OTA_METADATA_INSTALLING) {
        if (watch_ota_install_step(&install, &record) != WATCH_OTA_INSTALL_RESULT_OK) {
            (void)mark_error(&record, BOOTLOADER_OTA_ERROR_INSTALL);
            return -1;
        }
    }
    return record.state == WATCH_OTA_METADATA_TRIAL ? 1 : -1;
}
