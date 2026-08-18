#include "board/storage/watch_littlefs_board.h"

#include <string.h>

#include "board/storage/watch_w25q128_board.h"

#define WATCH_LITTLEFS_BOARD_IMAGE_BYTES 640U
#define WATCH_LITTLEFS_BOARD_FONT_BYTES 768U
#define WATCH_LITTLEFS_BOARD_TEXT_BYTES 2048U

typedef struct
{
    const uint8_t *expected;
    size_t expected_length;
    size_t position;
    uint32_t chunk_count;
} watch_littlefs_board_verify_t;

static watch_littlefs_t s_filesystem;
static bool s_initialized;
static watch_littlefs_result_t s_last_result = WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
static uint32_t s_image_chunks;
static uint32_t s_font_chunks;
static uint32_t s_text_chunks;
static uint8_t s_image[WATCH_LITTLEFS_BOARD_IMAGE_BYTES];
static uint8_t s_font[WATCH_LITTLEFS_BOARD_FONT_BYTES];
static uint8_t s_text[WATCH_LITTLEFS_BOARD_TEXT_BYTES];

static bool watch_littlefs_board_read(void *context, uint32_t address, uint8_t *data, size_t length)
{
    (void)context;
    return watch_w25q128_board_read(address, data, length) == WATCH_W25Q128_RESULT_OK;
}

static bool watch_littlefs_board_program(void *context, uint32_t address, const uint8_t *data,
                                         size_t length)
{
    (void)context;
    return watch_w25q128_board_page_program(address, data, length) == WATCH_W25Q128_RESULT_OK;
}

static bool watch_littlefs_board_erase(void *context, uint32_t address)
{
    (void)context;
    return watch_w25q128_board_sector_erase(address) == WATCH_W25Q128_RESULT_OK;
}

static bool watch_littlefs_board_sync(void *context)
{
    (void)context;
    return watch_w25q128_board_wait_ready(WATCH_W25Q128_BOARD_DEFAULT_TIMEOUT_MS)
        == WATCH_W25Q128_RESULT_OK;
}

static void watch_littlefs_board_fill_fixtures(void)
{
    static const char text_line[] = "F411 littlefs resource streaming fixture.\n";

    for (size_t index = 0U; index < sizeof(s_image); ++index) {
        s_image[index] = (uint8_t)((index * 13U) + 7U);
    }
    for (size_t index = 0U; index < sizeof(s_font); ++index) {
        s_font[index] = (uint8_t)((index * 29U) + 3U);
    }
    for (size_t index = 0U; index < sizeof(s_text); ++index) {
        s_text[index] = (uint8_t)text_line[index % (sizeof(text_line) - 1U)];
    }
}

static bool watch_littlefs_board_verify_chunk(void *context, const uint8_t *data, size_t length)
{
    watch_littlefs_board_verify_t *verify = (watch_littlefs_board_verify_t *)context;

    if (verify == NULL || data == NULL || length == 0U || verify->position > verify->expected_length
        || length > verify->expected_length - verify->position
        || memcmp(&verify->expected[verify->position], data, length) != 0) {
        return false;
    }

    verify->position += length;
    verify->chunk_count++;
    return true;
}

static watch_littlefs_result_t watch_littlefs_board_write_and_verify(const char *path,
                                                                     const uint8_t *expected,
                                                                     size_t expected_length,
                                                                     uint32_t *chunk_count)
{
    uint8_t buffer[WATCH_LITTLEFS_RESOURCE_CHUNK_SIZE] = { 0 };
    watch_littlefs_board_verify_t verify = {
        .expected = expected,
        .expected_length = expected_length,
    };
    watch_littlefs_result_t result;
    size_t total_length = 0U;

    result = watch_littlefs_write_file(&s_filesystem, path, expected, expected_length);
    if (result != WATCH_LITTLEFS_RESULT_OK) {
        return result;
    }

    result =
        watch_littlefs_read_file_chunks(&s_filesystem, path, buffer, sizeof(buffer),
                                        watch_littlefs_board_verify_chunk, &verify, &total_length);
    if (result != WATCH_LITTLEFS_RESULT_OK || verify.position != expected_length
        || total_length != expected_length) {
        return result == WATCH_LITTLEFS_RESULT_OK ? WATCH_LITTLEFS_RESULT_IO : result;
    }

    *chunk_count = verify.chunk_count;
    return WATCH_LITTLEFS_RESULT_OK;
}

bool watch_littlefs_board_init(void)
{
    watch_littlefs_backend_t backend;

    if (s_initialized) {
        return true;
    }

    backend = (watch_littlefs_backend_t) {
        .read = watch_littlefs_board_read,
        .program = watch_littlefs_board_program,
        .erase = watch_littlefs_board_erase,
        .sync = watch_littlefs_board_sync,
        .context = NULL,
    };
    s_initialized = watch_littlefs_init(&s_filesystem, &backend);
    if (!s_initialized) {
        s_last_result = WATCH_LITTLEFS_RESULT_INVALID_ARGUMENT;
    }
    return s_initialized;
}

watch_littlefs_result_t watch_littlefs_board_mount(void)
{
    if (!watch_littlefs_board_init()) {
        return s_last_result;
    }

    s_last_result = watch_littlefs_mount(&s_filesystem);
    return s_last_result;
}

watch_littlefs_result_t watch_littlefs_board_run_resource_test(void)
{
    watch_littlefs_result_t result;

    if (!watch_littlefs_board_init()) {
        return s_last_result;
    }

    watch_littlefs_board_fill_fixtures();
    s_image_chunks = 0U;
    s_font_chunks = 0U;
    s_text_chunks = 0U;
    result = watch_littlefs_format(&s_filesystem);
    if (result == WATCH_LITTLEFS_RESULT_OK) {
        result = watch_littlefs_board_write_and_verify("/image.bin", s_image, sizeof(s_image),
                                                       &s_image_chunks);
    }
    if (result == WATCH_LITTLEFS_RESULT_OK) {
        result = watch_littlefs_board_write_and_verify("/font.bin", s_font, sizeof(s_font),
                                                       &s_font_chunks);
    }
    if (result == WATCH_LITTLEFS_RESULT_OK) {
        result = watch_littlefs_board_write_and_verify("/long-text.txt", s_text, sizeof(s_text),
                                                       &s_text_chunks);
    }

    s_last_result = result;
    return s_last_result;
}

bool watch_littlefs_board_read_status(watch_littlefs_board_status_t *status)
{
    if (status == NULL || !s_initialized) {
        return false;
    }

    *status = (watch_littlefs_board_status_t) {
        .initialized = s_initialized,
        .mounted = watch_littlefs_is_mounted(&s_filesystem),
        .last_result = s_last_result,
        .image_chunks = s_image_chunks,
        .font_chunks = s_font_chunks,
        .text_chunks = s_text_chunks,
    };
    return true;
}
