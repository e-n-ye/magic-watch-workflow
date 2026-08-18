#include "watch_littlefs.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FAKE_FLASH_BLOCK_COUNT WATCH_LITTLEFS_BLOCK_COUNT
#define FAKE_FLASH_BYTES (FAKE_FLASH_BLOCK_COUNT * WATCH_LITTLEFS_BLOCK_SIZE)

typedef struct
{
    uint8_t memory[FAKE_FLASH_BYTES];
    uint32_t first_address;
    uint32_t last_address;
    uint32_t read_count;
    uint32_t program_count;
    uint32_t erase_count;
    bool fail_io;
} fake_flash_t;

typedef struct
{
    const uint8_t *expected;
    size_t expected_length;
    size_t position;
    uint32_t chunk_count;
} chunk_verify_t;

static fake_flash_t s_flash;

static void fake_flash_init(fake_flash_t *flash)
{
    memset(flash, 0, sizeof(*flash));
    memset(flash->memory, 0xFF, sizeof(flash->memory));
    flash->first_address = UINT32_MAX;
}

static bool fake_flash_range_valid(uint32_t address, size_t length)
{
    return watch_w25_littlefs_contains(address, length)
        && address - WATCH_W25_LITTLEFS_OFFSET <= FAKE_FLASH_BYTES
        && length <= FAKE_FLASH_BYTES - (address - WATCH_W25_LITTLEFS_OFFSET);
}

static void fake_flash_record(fake_flash_t *flash, uint32_t address, size_t length)
{
    if (address < flash->first_address) {
        flash->first_address = address;
    }
    if (address + length > flash->last_address) {
        flash->last_address = address + (uint32_t)length;
    }
}

static bool fake_flash_read(void *context, uint32_t address, uint8_t *data, size_t length)
{
    fake_flash_t *flash = (fake_flash_t *)context;

    assert(flash != NULL);
    assert(data != NULL);
    if (flash->fail_io || !fake_flash_range_valid(address, length)) {
        return false;
    }

    memcpy(data, &flash->memory[address - WATCH_W25_LITTLEFS_OFFSET], length);
    fake_flash_record(flash, address, length);
    flash->read_count++;
    return true;
}

static bool fake_flash_program(void *context, uint32_t address, const uint8_t *data, size_t length)
{
    fake_flash_t *flash = (fake_flash_t *)context;
    uint32_t offset;

    assert(flash != NULL);
    assert(data != NULL);
    assert(length <= WATCH_W25Q128_PAGE_SIZE);
    assert((address % WATCH_W25Q128_PAGE_SIZE) + length <= WATCH_W25Q128_PAGE_SIZE);
    if (flash->fail_io || !fake_flash_range_valid(address, length)) {
        return false;
    }

    offset = address - WATCH_W25_LITTLEFS_OFFSET;
    for (size_t index = 0U; index < length; ++index) {
        flash->memory[offset + index] &= data[index];
    }
    fake_flash_record(flash, address, length);
    flash->program_count++;
    return true;
}

static bool fake_flash_erase(void *context, uint32_t address)
{
    fake_flash_t *flash = (fake_flash_t *)context;

    assert(flash != NULL);
    assert((address % WATCH_W25Q128_SECTOR_SIZE) == 0U);
    if (flash->fail_io || !fake_flash_range_valid(address, WATCH_W25Q128_SECTOR_SIZE)) {
        return false;
    }

    memset(&flash->memory[address - WATCH_W25_LITTLEFS_OFFSET], 0xFF,
           WATCH_W25Q128_SECTOR_SIZE);
    fake_flash_record(flash, address, WATCH_W25Q128_SECTOR_SIZE);
    flash->erase_count++;
    return true;
}

static bool fake_flash_sync(void *context)
{
    fake_flash_t *flash = (fake_flash_t *)context;

    return flash != NULL && !flash->fail_io;
}

static bool initialize_filesystem(watch_littlefs_t *filesystem)
{
    watch_littlefs_backend_t backend = {
        .read = fake_flash_read,
        .program = fake_flash_program,
        .erase = fake_flash_erase,
        .sync = fake_flash_sync,
        .context = &s_flash,
    };

    return watch_littlefs_init(filesystem, &backend);
}

static bool verify_chunk(void *context, const uint8_t *data, size_t length)
{
    chunk_verify_t *verify = (chunk_verify_t *)context;

    if (verify == NULL || data == NULL || length == 0U || verify->position > verify->expected_length
        || length > verify->expected_length - verify->position
        || memcmp(&verify->expected[verify->position], data, length) != 0) {
        return false;
    }

    verify->position += length;
    verify->chunk_count++;
    return true;
}

static void test_partition_contract(void)
{
    assert(WATCH_W25_METADATA_OFFSET == 0x000000UL);
    assert(WATCH_W25_METADATA_END == 0x00FFFFUL);
    assert(WATCH_W25_CANDIDATE_OFFSET == 0x010000UL);
    assert(WATCH_W25_CANDIDATE_END == 0x08FFFFUL);
    assert(WATCH_W25_ROLLBACK_OFFSET == 0x090000UL);
    assert(WATCH_W25_ROLLBACK_END == 0x10FFFFUL);
    assert(WATCH_W25_LITTLEFS_OFFSET == 0x110000UL);
    assert(WATCH_W25_LITTLEFS_END == 0xFFFFFFUL);
    assert(WATCH_LITTLEFS_BLOCK_COUNT == 3824U);
    assert(watch_w25_littlefs_contains(WATCH_W25_LITTLEFS_OFFSET, 1U));
    assert(watch_w25_littlefs_contains(WATCH_W25_LITTLEFS_END, 1U));
    assert(!watch_w25_littlefs_contains(WATCH_W25_ROLLBACK_END, 1U));
    assert(!watch_w25_littlefs_contains(WATCH_W25_LITTLEFS_END, 2U));
}

static void test_format_mount_and_chunked_resources(void)
{
    uint8_t image[513];
    uint8_t font[769];
    uint8_t text[2051];
    uint8_t buffer[97];
    watch_littlefs_t filesystem;
    chunk_verify_t verify;
    size_t total_length = 0U;

    for (size_t index = 0U; index < sizeof(image); ++index) {
        image[index] = (uint8_t)((index * 13U) + 7U);
    }
    for (size_t index = 0U; index < sizeof(font); ++index) {
        font[index] = (uint8_t)((index * 29U) + 3U);
    }
    for (size_t index = 0U; index < sizeof(text); ++index) {
        text[index] = (uint8_t)('A' + (index % 26U));
    }

    fake_flash_init(&s_flash);
    assert(initialize_filesystem(&filesystem));
    assert(watch_littlefs_mount(&filesystem) == WATCH_LITTLEFS_RESULT_CORRUPT);
    assert(watch_littlefs_format(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_is_mounted(&filesystem));
    assert(watch_littlefs_write_file(&filesystem, "/image.bin", image, sizeof(image))
           == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_write_file(&filesystem, "/font.bin", font, sizeof(font))
           == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_write_file(&filesystem, "/long-text.txt", text, sizeof(text))
           == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_unmount(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_mount(&filesystem) == WATCH_LITTLEFS_RESULT_OK);

    verify = (chunk_verify_t) {
        .expected = image,
        .expected_length = sizeof(image),
    };
    assert(watch_littlefs_read_file_chunks(&filesystem, "/image.bin", buffer, sizeof(buffer),
                                           verify_chunk, &verify, &total_length)
           == WATCH_LITTLEFS_RESULT_OK);
    assert(verify.position == sizeof(image));
    assert(verify.chunk_count > 1U);
    assert(total_length == sizeof(image));

    verify = (chunk_verify_t) {
        .expected = font,
        .expected_length = sizeof(font),
    };
    assert(watch_littlefs_read_file_chunks(&filesystem, "/font.bin", buffer, sizeof(buffer),
                                           verify_chunk, &verify, &total_length)
           == WATCH_LITTLEFS_RESULT_OK);
    assert(verify.position == sizeof(font));
    assert(verify.chunk_count > 1U);
    assert(total_length == sizeof(font));

    verify = (chunk_verify_t) {
        .expected = text,
        .expected_length = sizeof(text),
    };
    assert(watch_littlefs_read_file_chunks(&filesystem, "/long-text.txt", buffer, sizeof(buffer),
                                           verify_chunk, &verify, &total_length)
           == WATCH_LITTLEFS_RESULT_OK);
    assert(verify.position == sizeof(text));
    assert(verify.chunk_count > 1U);
    assert(total_length == sizeof(text));
    assert(s_flash.read_count > 0U);
    assert(s_flash.program_count > 0U);
    assert(s_flash.erase_count > 0U);
    assert(s_flash.first_address >= WATCH_W25_LITTLEFS_OFFSET);
    assert(s_flash.last_address <= WATCH_W25Q128_CAPACITY_BYTES);
}

static void test_reader_bounds_and_backend_failure(void)
{
    uint8_t buffer[WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE + 1U];
    watch_littlefs_t filesystem;
    size_t total_length = 0U;

    fake_flash_init(&s_flash);
    assert(initialize_filesystem(&filesystem));
    assert(watch_littlefs_format(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_unmount(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    s_flash.fail_io = true;
    assert(watch_littlefs_mount(&filesystem) == WATCH_LITTLEFS_RESULT_IO);
    s_flash.fail_io = false;
    assert(watch_littlefs_mount(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_read_file_chunks(&filesystem, "/missing", buffer,
                                           WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE + 1U,
                                           verify_chunk, NULL, &total_length)
           == WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT);
    assert(watch_littlefs_read_file_chunks(&filesystem, "/missing", buffer, 1U, verify_chunk,
                                           &total_length, &total_length)
           == WATCH_LITTLEFS_RESULT_NOT_FOUND);
    assert(watch_littlefs_unmount(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_write_file(&filesystem, "/unmounted", buffer, 1U)
           == WATCH_LITTLEFS_RESULT_NOT_MOUNTED);
}

static void test_streaming_upload(void)
{
    uint8_t source[733];
    uint8_t buffer[97];
    watch_littlefs_t filesystem;
    watch_littlefs_upload_t upload;
    chunk_verify_t verify;
    size_t total_length = 0U;

    for (size_t index = 0U; index < sizeof(source); ++index) {
        source[index] = (uint8_t)(index * 7U + 1U);
    }
    fake_flash_init(&s_flash);
    assert(initialize_filesystem(&filesystem));
    assert(watch_littlefs_format(&filesystem) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_upload_begin(&filesystem, &upload, "/stream.bin")
           == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_upload_write(&upload, 0U, source, 317U) == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_upload_write(&upload, 317U, source + 317U, sizeof(source) - 317U)
           == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_upload_commit(&upload) == WATCH_LITTLEFS_RESULT_OK);
    verify = (chunk_verify_t) {
        .expected = source,
        .expected_length = sizeof(source),
    };
    assert(watch_littlefs_read_file_chunks(&filesystem, "/stream.bin", buffer, sizeof(buffer),
                                           verify_chunk, &verify, &total_length)
           == WATCH_LITTLEFS_RESULT_OK);
    assert(verify.position == sizeof(source));
    assert(total_length == sizeof(source));

    assert(watch_littlefs_upload_begin(&filesystem, &upload, "/aborted.bin")
           == WATCH_LITTLEFS_RESULT_OK);
    assert(watch_littlefs_upload_write(&upload, 0U, source, 1U) == WATCH_LITTLEFS_RESULT_OK);
    watch_littlefs_upload_abort(&upload);
    assert(watch_littlefs_read_file_chunks(&filesystem, "/aborted.bin", buffer, sizeof(buffer),
                                           verify_chunk, &verify, &total_length)
           == WATCH_LITTLEFS_RESULT_NOT_FOUND);
}

int main(void)
{
    test_partition_contract();
    test_format_mount_and_chunked_resources();
    test_reader_bounds_and_backend_failure();
    test_streaming_upload();
    assert(strcmp(watch_littlefs_result_name(WATCH_LITTLEFS_RESULT_CORRUPT), "corrupt") == 0);
    return 0;
}
