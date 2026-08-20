#include "watch_ota_install.h"

#include <string.h>

__attribute__((weak)) void watch_ota_install_progress(void) { }

static bool install_valid(const watch_ota_install_t *install)
{
    return install != NULL && install->initialized && install->config.read != NULL
        && install->config.erase != NULL && install->config.write != NULL
        && install->config.persist != NULL && install->config.context != NULL
        && install->config.app_size > 0U
        && install->config.rollback_size >= install->config.app_size
        && install->config.candidate_size >= install->config.app_size
        && install->config.sector_size > 0U
        && (install->config.sector_size % WATCH_OTA_INSTALL_BLOCK_SIZE) == 0U
        && (install->config.app_size % WATCH_OTA_INSTALL_BLOCK_SIZE) == 0U
        && (install->config.app_size % install->config.sector_size) == 0U;
}

static bool record_valid_for_install(const watch_ota_install_t *install,
                                     const watch_ota_metadata_record_t *record)
{
    return install_valid(install) && record != NULL && record->image_length > 0U
        && record->image_length <= install->config.candidate_size
        && record->candidate_counter >= record->confirmed_counter;
}

static bool persist_record(watch_ota_install_t *install, watch_ota_metadata_record_t *record,
                           const watch_ota_metadata_record_t *next)
{
    if (!install->config.persist(install->config.context, next)) {
        return false;
    }
    *record = *next;
    return true;
}

static watch_ota_install_result_t copy_block(watch_ota_install_t *install,
                                             watch_ota_install_region_t source,
                                             watch_ota_install_region_t destination,
                                             uint32_t offset, size_t length)
{
    uint8_t source_data[WATCH_OTA_INSTALL_BLOCK_SIZE];
    uint8_t destination_data[WATCH_OTA_INSTALL_BLOCK_SIZE];

    if (length == 0U || length > sizeof(source_data)) {
        return WATCH_OTA_INSTALL_RESULT_RANGE;
    }
    if ((offset % install->config.sector_size) == 0U
        && !install->config.erase(install->config.context, destination, offset,
                                  install->config.sector_size)) {
        return WATCH_OTA_INSTALL_RESULT_IO;
    }
    if (!install->config.read(install->config.context, source, offset, source_data, length)
        || !install->config.write(install->config.context, destination, offset, source_data, length)
        || !install->config.read(install->config.context, destination, offset, destination_data,
                                 length)) {
        return WATCH_OTA_INSTALL_RESULT_IO;
    }
    if (memcmp(source_data, destination_data, length) != 0) {
        return WATCH_OTA_INSTALL_RESULT_VERIFY;
    }
    return WATCH_OTA_INSTALL_RESULT_OK;
}

bool watch_ota_install_init(watch_ota_install_t *install, const watch_ota_install_config_t *config)
{
    if (install == NULL || config == NULL || config->read == NULL || config->erase == NULL
        || config->write == NULL || config->persist == NULL || config->context == NULL
        || config->app_size == 0U || config->rollback_size < config->app_size
        || config->candidate_size < config->app_size || config->sector_size == 0U
        || (config->sector_size % WATCH_OTA_INSTALL_BLOCK_SIZE) != 0U
        || (config->app_size % WATCH_OTA_INSTALL_BLOCK_SIZE) != 0U
        || (config->app_size % config->sector_size) != 0U) {
        return false;
    }

    *install = (watch_ota_install_t) {
        .config = *config,
        .initialized = true,
    };
    return true;
}

watch_ota_install_result_t watch_ota_install_start(watch_ota_install_t *install,
                                                   watch_ota_metadata_record_t *record)
{
    watch_ota_metadata_record_t next;

    if (!record_valid_for_install(install, record)) {
        return WATCH_OTA_INSTALL_RESULT_INVALID_ARGUMENT;
    }
    if (record->state != WATCH_OTA_METADATA_CANDIDATE_READY
        && record->state != WATCH_OTA_METADATA_PENDING_ROLLBACK) {
        return WATCH_OTA_INSTALL_RESULT_INVALID_STATE;
    }

    next = *record;
    next.state = record->state == WATCH_OTA_METADATA_PENDING_ROLLBACK
        ? WATCH_OTA_METADATA_ROLLING_BACK
        : WATCH_OTA_METADATA_BACKING_UP;
    next.progress = 0U;
    next.error_code = 0U;
    return persist_record(install, record, &next) ? WATCH_OTA_INSTALL_RESULT_OK
                                                  : WATCH_OTA_INSTALL_RESULT_PERSIST;
}

watch_ota_install_result_t watch_ota_install_step(watch_ota_install_t *install,
                                                  watch_ota_metadata_record_t *record)
{
    watch_ota_install_region_t source;
    watch_ota_install_region_t destination;
    watch_ota_metadata_record_t next;
    uint32_t offset;
    uint32_t remaining;
    size_t length;
    watch_ota_install_result_t result;

    watch_ota_install_progress();

    if (!record_valid_for_install(install, record)) {
        return WATCH_OTA_INSTALL_RESULT_INVALID_ARGUMENT;
    }
    if (record->state == WATCH_OTA_METADATA_TRIAL
        || record->state == WATCH_OTA_METADATA_CONFIRMED) {
        return WATCH_OTA_INSTALL_RESULT_COMPLETE;
    }
    if (record->state == WATCH_OTA_METADATA_BACKING_UP) {
        source = WATCH_OTA_INSTALL_REGION_APP;
        destination = WATCH_OTA_INSTALL_REGION_ROLLBACK;
    } else if (record->state == WATCH_OTA_METADATA_INSTALLING) {
        source = WATCH_OTA_INSTALL_REGION_CANDIDATE;
        destination = WATCH_OTA_INSTALL_REGION_APP;
    } else if (record->state == WATCH_OTA_METADATA_ROLLING_BACK) {
        source = WATCH_OTA_INSTALL_REGION_ROLLBACK;
        destination = WATCH_OTA_INSTALL_REGION_APP;
    } else {
        return WATCH_OTA_INSTALL_RESULT_INVALID_STATE;
    }

    if (record->progress > install->config.app_size
        || (record->progress % WATCH_OTA_INSTALL_BLOCK_SIZE) != 0U) {
        return WATCH_OTA_INSTALL_RESULT_INVALID_STATE;
    }
    offset = record->progress;
    remaining = install->config.app_size - offset;
    length = remaining > WATCH_OTA_INSTALL_BLOCK_SIZE ? WATCH_OTA_INSTALL_BLOCK_SIZE : remaining;
    result = copy_block(install, source, destination, offset, length);
    if (result != WATCH_OTA_INSTALL_RESULT_OK) {
        return result;
    }

    next = *record;
    if (offset + length == install->config.app_size) {
        if (record->state == WATCH_OTA_METADATA_BACKING_UP) {
            next.state = WATCH_OTA_METADATA_INSTALLING;
            next.progress = 0U;
        } else if (record->state == WATCH_OTA_METADATA_INSTALLING) {
            next.state = WATCH_OTA_METADATA_TRIAL;
            next.progress = install->config.app_size;
            next.trial_count = 0U;
        } else {
            next.state = WATCH_OTA_METADATA_CONFIRMED;
            next.progress = install->config.app_size;
            next.trial_count = 0U;
            next.error_code = 0U;
        }
    } else {
        next.progress = offset + (uint32_t)length;
    }

    return persist_record(install, record, &next) ? WATCH_OTA_INSTALL_RESULT_OK
                                                  : WATCH_OTA_INSTALL_RESULT_PERSIST;
}

const char *watch_ota_install_result_name(watch_ota_install_result_t result)
{
    switch (result) {
    case WATCH_OTA_INSTALL_RESULT_OK:
        return "ok";
    case WATCH_OTA_INSTALL_RESULT_COMPLETE:
        return "complete";
    case WATCH_OTA_INSTALL_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_OTA_INSTALL_RESULT_INVALID_STATE:
        return "state";
    case WATCH_OTA_INSTALL_RESULT_RANGE:
        return "range";
    case WATCH_OTA_INSTALL_RESULT_IO:
        return "io";
    case WATCH_OTA_INSTALL_RESULT_VERIFY:
        return "verify";
    case WATCH_OTA_INSTALL_RESULT_PERSIST:
        return "persist";
    case WATCH_OTA_INSTALL_RESULT_COUNT:
        return "invalid";
    }
    return "invalid";
}
