#include "watch_ota_metadata.h"

#include <string.h>

#define WATCH_OTA_METADATA_CRC_OFFSET 124U
#define WATCH_OTA_METADATA_READ_TIMEOUT_MS 1000U
#define WATCH_OTA_METADATA_WRITE_TIMEOUT_MS 5000U

_Static_assert(WATCH_OTA_METADATA_RECORD_SIZE <= WATCH_W25Q128_PAGE_SIZE,
               "metadata record must fit in one page");
_Static_assert(WATCH_OTA_METADATA_SLOT_COUNT *WATCH_OTA_METADATA_SLOT_SIZE
                   <= WATCH_W25_METADATA_SIZE,
               "metadata slots must fit in metadata partition");

static bool metadata_valid(const watch_ota_metadata_t *metadata)
{
    return metadata != NULL && metadata->initialized && metadata->flash != NULL
        && metadata->flash->initialized;
}

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) | ((uint32_t)data[2] << 16U)
        | ((uint32_t)data[3] << 24U);
}

static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;

    for (size_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xEDB88320UL : crc >> 1U;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

static bool state_valid(watch_ota_metadata_state_t state)
{
    return state >= WATCH_OTA_METADATA_DOWNLOADING && state <= WATCH_OTA_METADATA_ERROR;
}

static bool progress_valid(const watch_ota_metadata_record_t *record)
{
    if (record == NULL) {
        return false;
    }

    /* Install checkpoints cover the complete slot, including erased padding. */
    switch (record->state) {
    case WATCH_OTA_METADATA_BACKING_UP:
    case WATCH_OTA_METADATA_INSTALLING:
    case WATCH_OTA_METADATA_TRIAL:
    case WATCH_OTA_METADATA_CONFIRMED:
    case WATCH_OTA_METADATA_PENDING_ROLLBACK:
    case WATCH_OTA_METADATA_ROLLING_BACK:
    case WATCH_OTA_METADATA_ERROR:
        return record->progress <= WATCH_W25_CANDIDATE_SIZE;
    case WATCH_OTA_METADATA_DOWNLOADING:
    case WATCH_OTA_METADATA_CANDIDATE_READY:
        return record->progress <= record->image_length;
    }
    return false;
}

static bool record_fields_valid(const watch_ota_metadata_record_t *record)
{
    return record != NULL && record->sequence != 0U && state_valid(record->state)
        && record->candidate_counter >= record->confirmed_counter
        && record->image_length <= WATCH_W25_CANDIDATE_SIZE && progress_valid(record);
}

static void encode_record(const watch_ota_metadata_record_t *record,
                          uint8_t data[WATCH_OTA_METADATA_RECORD_SIZE])
{
    memset(data, 0, WATCH_OTA_METADATA_RECORD_SIZE);
    memcpy(data, "MWMD", 4U);
    write_u16(data + 4U, WATCH_OTA_METADATA_FORMAT_VERSION);
    write_u16(data + 6U, WATCH_OTA_METADATA_RECORD_SIZE);
    write_u32(data + 8U, record->sequence);
    write_u32(data + 12U, (uint32_t)record->state);
    write_u32(data + 16U, record->confirmed_counter);
    write_u32(data + 20U, record->candidate_counter);
    write_u32(data + 24U, record->active_version);
    write_u32(data + 28U, record->candidate_version);
    write_u32(data + 32U, record->image_length);
    write_u32(data + 36U, record->progress);
    write_u32(data + 40U, record->trial_count);
    write_u32(data + 44U, record->error_code);
    memcpy(data + 48U, record->active_digest, WATCH_OTA_METADATA_DIGEST_SIZE);
    memcpy(data + 80U, record->candidate_digest, WATCH_OTA_METADATA_DIGEST_SIZE);
    write_u32(data + WATCH_OTA_METADATA_CRC_OFFSET, crc32(data, WATCH_OTA_METADATA_CRC_OFFSET));
}

static bool decode_record(const uint8_t data[WATCH_OTA_METADATA_RECORD_SIZE],
                          watch_ota_metadata_record_t *record)
{
    uint32_t stored_crc;

    if (memcmp(data, "MWMD", 4U) != 0 || read_u16(data + 4U) != WATCH_OTA_METADATA_FORMAT_VERSION
        || read_u16(data + 6U) != WATCH_OTA_METADATA_RECORD_SIZE) {
        return false;
    }

    stored_crc = read_u32(data + WATCH_OTA_METADATA_CRC_OFFSET);
    if (stored_crc != crc32(data, WATCH_OTA_METADATA_CRC_OFFSET)) {
        return false;
    }

    memset(record, 0, sizeof(*record));
    record->sequence = read_u32(data + 8U);
    record->state = (watch_ota_metadata_state_t)read_u32(data + 12U);
    record->confirmed_counter = read_u32(data + 16U);
    record->candidate_counter = read_u32(data + 20U);
    record->active_version = read_u32(data + 24U);
    record->candidate_version = read_u32(data + 28U);
    record->image_length = read_u32(data + 32U);
    record->progress = read_u32(data + 36U);
    record->trial_count = read_u32(data + 40U);
    record->error_code = read_u32(data + 44U);
    memcpy(record->active_digest, data + 48U, WATCH_OTA_METADATA_DIGEST_SIZE);
    memcpy(record->candidate_digest, data + 80U, WATCH_OTA_METADATA_DIGEST_SIZE);
    return record_fields_valid(record);
}

static bool record_blank(const uint8_t data[WATCH_OTA_METADATA_RECORD_SIZE])
{
    for (size_t index = 0U; index < WATCH_OTA_METADATA_RECORD_SIZE; ++index) {
        if (data[index] != 0xFFU) {
            return false;
        }
    }
    return true;
}

typedef struct
{
    watch_ota_metadata_record_t record;
    unsigned int slot;
} metadata_scan_result_t;

static watch_ota_metadata_result_t scan_metadata(const watch_ota_metadata_t *metadata,
                                                 metadata_scan_result_t *scan)
{
    bool found = false;
    bool corrupt = false;

    if (!metadata_valid(metadata) || scan == NULL) {
        return WATCH_OTA_METADATA_RESULT_INVALID_ARGUMENT;
    }

    memset(scan, 0, sizeof(*scan));
    for (unsigned int slot = 0U; slot < WATCH_OTA_METADATA_SLOT_COUNT; ++slot) {
        uint8_t raw[WATCH_OTA_METADATA_RECORD_SIZE];
        watch_ota_metadata_record_t candidate;
        watch_w25q128_result_t result =
            watch_w25q128_read(metadata->flash, WATCH_OTA_METADATA_SLOT_OFFSET(slot), raw,
                               sizeof(raw), WATCH_OTA_METADATA_READ_TIMEOUT_MS);

        if (result != WATCH_W25Q128_RESULT_OK) {
            return WATCH_OTA_METADATA_RESULT_IO;
        }
        if (record_blank(raw)) {
            continue;
        }
        if (!decode_record(raw, &candidate)) {
            corrupt = true;
            continue;
        }
        if (!found || candidate.sequence > scan->record.sequence) {
            scan->record = candidate;
            scan->slot = slot;
            found = true;
        }
    }

    if (found) {
        return WATCH_OTA_METADATA_RESULT_OK;
    }
    return corrupt ? WATCH_OTA_METADATA_RESULT_CORRUPT : WATCH_OTA_METADATA_RESULT_EMPTY;
}

static bool same_digest(const uint8_t left[WATCH_OTA_METADATA_DIGEST_SIZE],
                        const uint8_t right[WATCH_OTA_METADATA_DIGEST_SIZE])
{
    return memcmp(left, right, WATCH_OTA_METADATA_DIGEST_SIZE) == 0;
}

static watch_ota_metadata_result_t security_policy(const watch_ota_metadata_record_t *current,
                                                   const watch_ota_metadata_record_t *incoming)
{
    uint32_t floor = current->confirmed_counter > current->candidate_counter
        ? current->confirmed_counter
        : current->candidate_counter;

    if (incoming->confirmed_counter < current->confirmed_counter
        || incoming->candidate_counter < floor) {
        return WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED;
    }
    if (incoming->candidate_counter == floor
        && !same_digest(incoming->candidate_digest, current->candidate_digest)
        && !same_digest(incoming->candidate_digest, current->active_digest)) {
        return WATCH_OTA_METADATA_RESULT_CONFLICT;
    }
    return WATCH_OTA_METADATA_RESULT_OK;
}

bool watch_ota_metadata_init(watch_ota_metadata_t *metadata, watch_w25q128_t *flash)
{
    if (metadata == NULL || flash == NULL || !flash->initialized) {
        return false;
    }

    *metadata = (watch_ota_metadata_t) {
        .flash = flash,
        .initialized = true,
    };
    return true;
}

watch_ota_metadata_result_t watch_ota_metadata_load(const watch_ota_metadata_t *metadata,
                                                    watch_ota_metadata_record_t *record)
{
    metadata_scan_result_t scan;
    watch_ota_metadata_result_t result;

    if (record == NULL) {
        return WATCH_OTA_METADATA_RESULT_INVALID_ARGUMENT;
    }
    result = scan_metadata(metadata, &scan);
    if (result == WATCH_OTA_METADATA_RESULT_OK) {
        *record = scan.record;
    }
    return result;
}

watch_ota_metadata_result_t watch_ota_metadata_commit(watch_ota_metadata_t *metadata,
                                                      const watch_ota_metadata_record_t *record)
{
    metadata_scan_result_t scan;
    watch_ota_metadata_record_t stored;
    uint8_t raw[WATCH_OTA_METADATA_RECORD_SIZE];
    watch_ota_metadata_result_t current_result;
    unsigned int target_slot = 0U;
    uint32_t sequence = 1U;
    watch_w25q128_result_t flash_result;

    if (!metadata_valid(metadata) || record == NULL) {
        return WATCH_OTA_METADATA_RESULT_INVALID_ARGUMENT;
    }
    if (!state_valid(record->state) || record->candidate_counter < record->confirmed_counter
        || record->image_length > WATCH_W25_CANDIDATE_SIZE || !progress_valid(record)) {
        return WATCH_OTA_METADATA_RESULT_INVALID_RECORD;
    }

    current_result = scan_metadata(metadata, &scan);
    if (current_result == WATCH_OTA_METADATA_RESULT_OK) {
        watch_ota_metadata_result_t policy_result = security_policy(&scan.record, record);
        if (policy_result != WATCH_OTA_METADATA_RESULT_OK) {
            return policy_result;
        }
        if (scan.record.sequence == UINT32_MAX) {
            return WATCH_OTA_METADATA_RESULT_INVALID_RECORD;
        }
        sequence = scan.record.sequence + 1U;
        target_slot = scan.slot ^ 1U;
    } else if (current_result == WATCH_OTA_METADATA_RESULT_EMPTY) {
        target_slot = 0U;
    } else {
        return current_result;
    }

    stored = *record;
    stored.sequence = sequence;
    encode_record(&stored, raw);
    flash_result =
        watch_w25q128_sector_erase(metadata->flash, WATCH_OTA_METADATA_SLOT_OFFSET(target_slot),
                                   WATCH_OTA_METADATA_WRITE_TIMEOUT_MS);
    if (flash_result != WATCH_W25Q128_RESULT_OK) {
        return WATCH_OTA_METADATA_RESULT_IO;
    }
    flash_result =
        watch_w25q128_page_program(metadata->flash, WATCH_OTA_METADATA_SLOT_OFFSET(target_slot),
                                   raw, sizeof(raw), WATCH_OTA_METADATA_WRITE_TIMEOUT_MS);
    if (flash_result != WATCH_W25Q128_RESULT_OK) {
        return WATCH_OTA_METADATA_RESULT_IO;
    }

    memset(&stored, 0, sizeof(stored));
    if (watch_w25q128_read(metadata->flash, WATCH_OTA_METADATA_SLOT_OFFSET(target_slot), raw,
                           sizeof(raw), WATCH_OTA_METADATA_READ_TIMEOUT_MS)
            != WATCH_W25Q128_RESULT_OK
        || !decode_record(raw, &stored) || stored.sequence != sequence) {
        return WATCH_OTA_METADATA_RESULT_IO;
    }
    return WATCH_OTA_METADATA_RESULT_OK;
}

const char *watch_ota_metadata_state_name(watch_ota_metadata_state_t state)
{
    switch (state) {
    case WATCH_OTA_METADATA_DOWNLOADING:
        return "downloading";
    case WATCH_OTA_METADATA_CANDIDATE_READY:
        return "candidate-ready";
    case WATCH_OTA_METADATA_BACKING_UP:
        return "backing-up";
    case WATCH_OTA_METADATA_INSTALLING:
        return "installing";
    case WATCH_OTA_METADATA_TRIAL:
        return "trial";
    case WATCH_OTA_METADATA_CONFIRMED:
        return "confirmed";
    case WATCH_OTA_METADATA_PENDING_ROLLBACK:
        return "pending-rollback";
    case WATCH_OTA_METADATA_ROLLING_BACK:
        return "rolling-back";
    case WATCH_OTA_METADATA_ERROR:
        return "error";
    }
    return "invalid";
}

const char *watch_ota_metadata_result_name(watch_ota_metadata_result_t result)
{
    switch (result) {
    case WATCH_OTA_METADATA_RESULT_OK:
        return "ok";
    case WATCH_OTA_METADATA_RESULT_EMPTY:
        return "empty";
    case WATCH_OTA_METADATA_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_OTA_METADATA_RESULT_IO:
        return "io";
    case WATCH_OTA_METADATA_RESULT_CORRUPT:
        return "corrupt";
    case WATCH_OTA_METADATA_RESULT_INVALID_RECORD:
        return "record";
    case WATCH_OTA_METADATA_RESULT_SECURITY_REJECTED:
        return "security";
    case WATCH_OTA_METADATA_RESULT_CONFLICT:
        return "conflict";
    case WATCH_OTA_METADATA_RESULT_COUNT:
        return "invalid";
    }
    return "invalid";
}
