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
static watch_ymodem_t s_ymodem;
static watch_ota_metadata_record_t s_download_record;
static watch_ota_download_read_fn s_download_read;
static watch_ota_download_write_fn s_download_write;
static void *s_download_context;
static watch_ota_download_channel_t s_download_channel;
static bool s_download_active;
static bool s_download_preparing;
static uint32_t s_download_erase_offset;
static uint32_t s_download_bytes;
static uint32_t s_download_last_persist;

#define WATCH_OTA_DOWNLOAD_READ_SIZE 128U
#define WATCH_OTA_DOWNLOAD_PROGRESS_GRANULARITY 4096U

static bool ota_metadata_record_valid_for_download(const watch_ota_metadata_record_t *record)
{
    return record != NULL && record->state != WATCH_OTA_METADATA_BACKING_UP
        && record->state != WATCH_OTA_METADATA_INSTALLING
        && record->state != WATCH_OTA_METADATA_TRIAL
        && record->state != WATCH_OTA_METADATA_PENDING_ROLLBACK
        && record->state != WATCH_OTA_METADATA_ROLLING_BACK;
}

static bool ota_emit(void *context, uint8_t byte)
{
    (void)context;
    return s_download_write != NULL && s_download_write(s_download_context, &byte, 1U) == 1U;
}

static bool ota_candidate_write(uint32_t offset, const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0U || offset > WATCH_OTA_PACKAGE_SIZE
        || length > WATCH_OTA_PACKAGE_SIZE - offset) {
        return false;
    }

    while (length > 0U) {
        uint32_t address = WATCH_W25_CANDIDATE_OFFSET + offset;
        size_t page_length = WATCH_W25Q128_PAGE_SIZE - (address % WATCH_W25Q128_PAGE_SIZE);
        size_t chunk = length < page_length ? length : page_length;

        if (watch_w25q128_board_page_program(address, data, chunk) != WATCH_W25Q128_RESULT_OK) {
            return false;
        }
        offset += (uint32_t)chunk;
        data += chunk;
        length -= chunk;
    }
    return true;
}

static bool ota_persist_download_record(void)
{
    return watch_ota_metadata_commit(&s_metadata, &s_download_record)
        == WATCH_OTA_METADATA_RESULT_OK;
}

static bool ota_download_begin(void *context, const char *name, uint32_t size)
{
    (void)context;
    (void)name;

    if (size != WATCH_OTA_PACKAGE_SIZE) {
        return false;
    }
    s_download_record.image_length = size;
    s_download_record.progress = 0U;
    s_download_bytes = 0U;
    s_download_last_persist = 0U;
    return ota_persist_download_record();
}

static bool ota_download_data(void *context, uint32_t offset, const uint8_t *data, size_t length)
{
    (void)context;

    if (offset != s_download_bytes || !ota_candidate_write(offset, data, length)) {
        return false;
    }
    s_download_bytes += (uint32_t)length;
    if (s_download_bytes == s_download_record.image_length
        || s_download_bytes - s_download_last_persist >= WATCH_OTA_DOWNLOAD_PROGRESS_GRANULARITY) {
        s_download_record.progress = s_download_bytes;
        if (!ota_persist_download_record()) {
            return false;
        }
        s_download_last_persist = s_download_bytes;
    }
    return true;
}

static bool ota_download_commit(void *context)
{
    (void)context;
    return watch_ota_board_stage_candidate() == WATCH_OTA_METADATA_RESULT_OK;
}

static void ota_download_abort(void *context)
{
    (void)context;
    s_download_record.state = WATCH_OTA_METADATA_ERROR;
    s_download_record.error_code = (uint32_t)watch_ymodem_result(&s_ymodem);
    s_download_record.progress = s_download_bytes;
    (void)ota_persist_download_record();
}

static bool ota_erase_candidate_sector(void)
{
    if (s_download_erase_offset >= WATCH_OTA_PACKAGE_SIZE) {
        return true;
    }
    if (watch_w25q128_board_sector_erase(WATCH_W25_CANDIDATE_OFFSET + s_download_erase_offset)
        != WATCH_W25Q128_RESULT_OK) {
        return false;
    }
    s_download_erase_offset += WATCH_W25Q128_SECTOR_SIZE;
    return true;
}

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

bool watch_ota_board_reset_metadata(void)
{
    if (!watch_ota_board_init()) {
        return false;
    }
    for (unsigned int slot = 0U; slot < WATCH_OTA_METADATA_SLOT_COUNT; ++slot) {
        if (watch_w25q128_board_sector_erase(WATCH_OTA_METADATA_SLOT_OFFSET(slot))
            != WATCH_W25Q128_RESULT_OK) {
            return false;
        }
    }
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
    status->download_active = s_download_active;
    status->download_channel = s_download_channel;
    status->download_state = watch_ymodem_state(&s_ymodem);
    status->download_result = watch_ymodem_result(&s_ymodem);
    return true;
}

bool watch_ota_board_start_download(watch_ota_download_channel_t channel,
                                    watch_ota_download_read_fn read,
                                    watch_ota_download_write_fn write, void *context)
{
    watch_ota_metadata_record_t current;
    watch_ota_metadata_result_t metadata_result;
    watch_ymodem_sink_t sink = {
        .begin = ota_download_begin,
        .data = ota_download_data,
        .commit = ota_download_commit,
        .abort = ota_download_abort,
        .context = NULL,
    };

    if (channel >= WATCH_OTA_DOWNLOAD_CHANNEL_COUNT || read == NULL || write == NULL
        || s_download_active || !watch_ota_board_init()) {
        return false;
    }
    metadata_result = watch_ota_metadata_load(&s_metadata, &current);
    if (metadata_result == WATCH_OTA_METADATA_RESULT_CORRUPT
        || (metadata_result == WATCH_OTA_METADATA_RESULT_OK
            && !ota_metadata_record_valid_for_download(&current))) {
        return false;
    }
    if (metadata_result == WATCH_OTA_METADATA_RESULT_EMPTY) {
        memset(&current, 0, sizeof(current));
    }
    current.state = WATCH_OTA_METADATA_DOWNLOADING;
    current.image_length = WATCH_OTA_PACKAGE_SIZE;
    current.progress = 0U;
    current.error_code = 0U;
    if (watch_ota_metadata_commit(&s_metadata, &current) != WATCH_OTA_METADATA_RESULT_OK) {
        return false;
    }
    s_download_record = current;

    s_download_read = read;
    s_download_write = write;
    s_download_context = context;
    s_download_channel = channel;
    s_download_preparing = true;
    s_download_erase_offset = 0U;
    s_download_bytes = 0U;
    s_download_last_persist = 0U;
    if (!watch_ymodem_init(&s_ymodem, &sink, ota_emit, NULL)) {
        s_download_read = NULL;
        s_download_write = NULL;
        s_download_context = NULL;
        s_download_preparing = false;
        return false;
    }
    s_download_active = true;
    return true;
}

size_t watch_ota_board_process_download(void)
{
    uint8_t buffer[WATCH_OTA_DOWNLOAD_READ_SIZE];
    size_t length;

    if (!s_download_active || s_download_read == NULL) {
        return 0U;
    }
    if (s_download_preparing) {
        if (!ota_erase_candidate_sector()) {
            s_download_record.state = WATCH_OTA_METADATA_ERROR;
            s_download_record.error_code = WATCH_OTA_METADATA_RESULT_IO;
            (void)ota_persist_download_record();
            s_download_active = false;
            s_download_preparing = false;
            s_download_read = NULL;
            s_download_write = NULL;
            s_download_context = NULL;
            return 0U;
        }
        if (s_download_erase_offset >= WATCH_OTA_PACKAGE_SIZE) {
            s_download_preparing = false;
            if (watch_ymodem_start(&s_ymodem) != WATCH_YMODEM_RESULT_OK) {
                ota_download_abort(NULL);
                s_download_active = false;
                s_download_read = NULL;
                s_download_write = NULL;
                s_download_context = NULL;
            }
        }
        return 0U;
    }
    length = s_download_read(s_download_context, buffer, sizeof(buffer));
    if (length > 0U) {
        (void)watch_ymodem_feed(&s_ymodem, buffer, length);
    }
    if (watch_ymodem_state(&s_ymodem) == WATCH_YMODEM_STATE_COMPLETE
        || watch_ymodem_state(&s_ymodem) == WATCH_YMODEM_STATE_ERROR) {
        s_download_active = false;
        s_download_preparing = false;
        s_download_read = NULL;
        s_download_write = NULL;
        s_download_context = NULL;
    }
    return length;
}

bool watch_ota_board_download_active(void)
{
    return s_download_active;
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

watch_ota_metadata_result_t watch_ota_board_confirm_trial(void)
{
    watch_ota_metadata_record_t record;
    watch_ota_metadata_result_t metadata_result;

    if (!watch_ota_board_init()) {
        return WATCH_OTA_METADATA_RESULT_IO;
    }
    metadata_result = watch_ota_metadata_load(&s_metadata, &record);
    if (metadata_result != WATCH_OTA_METADATA_RESULT_OK) {
        return metadata_result;
    }
    if (watch_ota_trial_confirm(&record) != WATCH_OTA_TRIAL_RESULT_OK) {
        return WATCH_OTA_METADATA_RESULT_INVALID_RECORD;
    }
    return watch_ota_metadata_commit(&s_metadata, &record);
}

watch_ota_metadata_result_t watch_ota_board_mark_trial_fault(uint32_t error_code)
{
    watch_ota_metadata_record_t record;
    watch_ota_metadata_result_t metadata_result;

    if (!watch_ota_board_init()) {
        return WATCH_OTA_METADATA_RESULT_IO;
    }
    metadata_result = watch_ota_metadata_load(&s_metadata, &record);
    if (metadata_result != WATCH_OTA_METADATA_RESULT_OK) {
        return metadata_result;
    }
    if (watch_ota_trial_mark_fault(&record, error_code) != WATCH_OTA_TRIAL_RESULT_OK) {
        return WATCH_OTA_METADATA_RESULT_INVALID_RECORD;
    }
    return watch_ota_metadata_commit(&s_metadata, &record);
}
