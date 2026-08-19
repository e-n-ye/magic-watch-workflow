#include "board/storage/watch_ota_board.h"

#include <string.h>

#include "board/storage/watch_w25q128_board.h"

/* Kept private to this OTA adapter; the application only sees semantic APIs. */
extern watch_w25q128_t *watch_w25q128_board_device(void);

static const uint8_t s_ota_public_key[WATCH_OTA_PACKAGE_SIGNATURE_SIZE] = {
    0xa2, 0xbe, 0xdb, 0x37, 0x4d, 0x14, 0xf8, 0xae, 0x2c, 0xbb, 0xfe, 0x23, 0x36, 0x4e, 0x36, 0xea,
    0x5a, 0x39, 0x1d, 0xb1, 0x14, 0x72, 0x82, 0x17, 0x25, 0x6b, 0xe3, 0x9f, 0x44, 0x7d, 0x40, 0x69,
    0xf2, 0x0b, 0x59, 0x08, 0xce, 0xa6, 0xdc, 0x76, 0xe7, 0x29, 0x52, 0x3f, 0xce, 0xac, 0x10, 0x5a,
    0xf9, 0x94, 0x5b, 0x55, 0xc0, 0x53, 0x51, 0x73, 0x5a, 0x93, 0x8e, 0x2b, 0xe6, 0xb5, 0xc8, 0xbe,
};

static watch_ota_metadata_t s_metadata;
static bool s_initialized;

static bool candidate_read(void *context, uint32_t offset, uint8_t *data, size_t length)
{
    (void)context;

    if (data == NULL
        || !watch_w25_partition_contains(WATCH_W25_CANDIDATE_OFFSET, WATCH_W25_CANDIDATE_SIZE,
                                         WATCH_W25_CANDIDATE_OFFSET + offset, length)) {
        return false;
    }

    return watch_w25q128_board_read(WATCH_W25_CANDIDATE_OFFSET + offset, data, length)
        == WATCH_W25Q128_RESULT_OK;
}

static uint32_t metadata_counter_floor(const watch_ota_metadata_record_t *record)
{
    if (record == NULL) {
        return 0U;
    }
    return record->confirmed_counter > record->candidate_counter ? record->confirmed_counter
                                                                 : record->candidate_counter;
}

bool watch_ota_board_init(void)
{
    watch_w25q128_t *device;

    if (s_initialized) {
        return true;
    }
    if (!watch_w25q128_board_init()) {
        return false;
    }
    device = watch_w25q128_board_device();
    if (device == NULL || !watch_ota_metadata_init(&s_metadata, device)) {
        return false;
    }
    s_initialized = true;
    return true;
}

bool watch_ota_board_read_status(watch_ota_board_status_t *status)
{
    watch_ota_metadata_result_t metadata_result;
    uint32_t jedec_id = 0U;
    watch_w25q128_result_t flash_result;

    if (status == NULL || !watch_ota_board_init()) {
        return false;
    }

    flash_result = watch_w25q128_board_read_id(&jedec_id);
    *status = (watch_ota_board_status_t) {
        .initialized = true,
        .jedec_id = jedec_id,
        .flash_result = flash_result,
    };
    metadata_result = watch_ota_metadata_load(&s_metadata, &status->record);
    status->metadata_result = metadata_result;
    status->record_valid = metadata_result == WATCH_OTA_METADATA_RESULT_OK;
    return true;
}

watch_ota_package_result_t watch_ota_board_verify_candidate(watch_ota_package_info_t *info)
{
    watch_ota_metadata_record_t record;
    watch_ota_package_reader_t reader = {
        .read = candidate_read,
        .context = NULL,
        .size = WATCH_OTA_PACKAGE_SIZE,
    };
    watch_ota_metadata_result_t metadata_result;
    uint32_t minimum_counter = 0U;

    if (info == NULL || !watch_ota_board_init()) {
        return WATCH_OTA_PACKAGE_RESULT_INVALID_ARGUMENT;
    }
    metadata_result = watch_ota_metadata_load(&s_metadata, &record);
    if (metadata_result == WATCH_OTA_METADATA_RESULT_OK) {
        minimum_counter = metadata_counter_floor(&record);
    } else if (metadata_result != WATCH_OTA_METADATA_RESULT_EMPTY) {
        return WATCH_OTA_PACKAGE_RESULT_IO;
    }

    return watch_ota_package_verify(&reader, WATCH_OTA_PACKAGE_BOARD_ID, minimum_counter,
                                    s_ota_public_key, info);
}

watch_ota_metadata_result_t watch_ota_board_stage_candidate(void)
{
    watch_ota_metadata_record_t current;
    watch_ota_metadata_record_t next;
    watch_ota_package_info_t info;
    watch_ota_metadata_result_t metadata_result;
    watch_ota_package_result_t package_result;

    if (!watch_ota_board_init()) {
        return WATCH_OTA_METADATA_RESULT_IO;
    }
    package_result = watch_ota_board_verify_candidate(&info);
    if (package_result != WATCH_OTA_PACKAGE_RESULT_OK) {
        return WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED;
    }

    metadata_result = watch_ota_metadata_load(&s_metadata, &current);
    if (metadata_result == WATCH_OTA_METADATA_RESULT_OK) {
        next = current;
    } else if (metadata_result == WATCH_OTA_METADATA_RESULT_EMPTY) {
        memset(&next, 0, sizeof(next));
    } else {
        return metadata_result;
    }

    next.state = WATCH_OTA_METADATA_CANDIDATE_READY;
    next.candidate_counter = info.security_counter;
    next.candidate_version = info.firmware_version;
    next.image_length = info.image_length;
    next.progress = 0U;
    next.trial_count = 0U;
    next.error_code = 0U;
    memcpy(next.candidate_digest, info.digest, sizeof(next.candidate_digest));
    return watch_ota_metadata_commit(&s_metadata, &next);
}
