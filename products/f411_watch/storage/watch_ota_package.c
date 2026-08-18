#include "watch_ota_package.h"

#include <string.h>

#include "tinycrypt/constants.h"
#include "tinycrypt/ecc_dsa.h"
#include "tinycrypt/sha256.h"

#define WATCH_OTA_PACKAGE_CHUNK_SIZE 256U

int default_CSPRNG(uint8_t *destination, unsigned int size)
{
    (void)destination;
    (void)size;
    return 0;
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

static bool erased(const uint8_t *data, size_t length)
{
    for (size_t index = 0U; index < length; ++index) {
        if (data[index] != 0xFFU) {
            return false;
        }
    }
    return true;
}

static watch_ota_package_result_t check_padding(const watch_ota_package_reader_t *reader,
                                                uint32_t offset, uint32_t length)
{
    uint8_t buffer[WATCH_OTA_PACKAGE_CHUNK_SIZE];

    while (length > 0U) {
        size_t chunk = length > sizeof(buffer) ? sizeof(buffer) : length;
        if (!reader->read(reader->context, offset, buffer, chunk)) {
            return WATCH_OTA_PACKAGE_RESULT_IO;
        }
        if (!erased(buffer, chunk)) {
            return WATCH_OTA_PACKAGE_RESULT_PADDING;
        }
        offset += (uint32_t)chunk;
        length -= (uint32_t)chunk;
    }
    return WATCH_OTA_PACKAGE_RESULT_OK;
}

watch_ota_package_result_t
watch_ota_package_verify(const watch_ota_package_reader_t *reader, uint32_t expected_board,
                         uint32_t minimum_security_counter,
                         const uint8_t public_key[WATCH_OTA_PACKAGE_SIGNATURE_SIZE],
                         watch_ota_package_info_t *info)
{
    uint8_t header[WATCH_OTA_PACKAGE_HEADER_SIZE];
    uint8_t buffer[WATCH_OTA_PACKAGE_CHUNK_SIZE];
    uint8_t digest[WATCH_OTA_PACKAGE_DIGEST_SIZE];
    uint8_t signed_digest[WATCH_OTA_PACKAGE_DIGEST_SIZE];
    struct tc_sha256_state_struct state;
    uint32_t image_length;
    uint32_t offset;
    watch_ota_package_result_t result;

    if (reader == NULL || reader->read == NULL || public_key == NULL || info == NULL) {
        return WATCH_OTA_PACKAGE_RESULT_INVALID_ARGUMENT;
    }
    if (reader->size != WATCH_OTA_PACKAGE_SIZE) {
        return WATCH_OTA_PACKAGE_RESULT_SIZE;
    }
    if (!reader->read(reader->context, WATCH_OTA_PACKAGE_TRAILER_OFFSET, header, sizeof(header))) {
        return WATCH_OTA_PACKAGE_RESULT_IO;
    }
    if (memcmp(header, "MWMF", 4U) != 0 || read_u16(header + 4U) != WATCH_OTA_PACKAGE_FORMAT_VERSION
        || read_u16(header + 6U) != WATCH_OTA_PACKAGE_HEADER_SIZE) {
        return WATCH_OTA_PACKAGE_RESULT_FORMAT;
    }
    if (read_u32(header + 8U) != expected_board) {
        return WATCH_OTA_PACKAGE_RESULT_BOARD;
    }
    if (read_u32(header + 20U) != WATCH_OTA_PACKAGE_LOAD_ADDRESS) {
        return WATCH_OTA_PACKAGE_RESULT_FORMAT;
    }
    image_length = read_u32(header + 24U);
    if (image_length == 0U || image_length > WATCH_OTA_PACKAGE_TRAILER_OFFSET) {
        return WATCH_OTA_PACKAGE_RESULT_RANGE;
    }
    if (header[60] != WATCH_OTA_PACKAGE_KEY_ID || header[61] != 0U || header[62] != 0U
        || header[63] != 0U || erased(header + 64U, WATCH_OTA_PACKAGE_SIGNATURE_SIZE)) {
        return WATCH_OTA_PACKAGE_RESULT_FORMAT;
    }
    if (read_u32(header + 16U) < minimum_security_counter) {
        return WATCH_OTA_PACKAGE_RESULT_SECURITY_REJECTED;
    }

    result = check_padding(reader, image_length, WATCH_OTA_PACKAGE_TRAILER_OFFSET - image_length);
    if (result != WATCH_OTA_PACKAGE_RESULT_OK) {
        return result;
    }
    result = check_padding(reader, WATCH_OTA_PACKAGE_TRAILER_OFFSET + WATCH_OTA_PACKAGE_HEADER_SIZE,
                           WATCH_OTA_PACKAGE_TRAILER_SIZE - WATCH_OTA_PACKAGE_HEADER_SIZE);
    if (result != WATCH_OTA_PACKAGE_RESULT_OK) {
        return result;
    }

    if (tc_sha256_init(&state) != TC_CRYPTO_SUCCESS) {
        return WATCH_OTA_PACKAGE_RESULT_HASH;
    }
    offset = 0U;
    while (offset < image_length) {
        size_t chunk =
            image_length - offset > sizeof(buffer) ? sizeof(buffer) : image_length - offset;
        if (!reader->read(reader->context, offset, buffer, chunk)
            || tc_sha256_update(&state, buffer, chunk) != TC_CRYPTO_SUCCESS) {
            return WATCH_OTA_PACKAGE_RESULT_IO;
        }
        offset += (uint32_t)chunk;
    }
    if (tc_sha256_final(digest, &state) != TC_CRYPTO_SUCCESS
        || memcmp(digest, header + 28U, sizeof(digest)) != 0) {
        return WATCH_OTA_PACKAGE_RESULT_HASH;
    }
    if (tc_sha256_init(&state) != TC_CRYPTO_SUCCESS
        || tc_sha256_update(&state, header, WATCH_OTA_PACKAGE_SIGNED_SIZE) != TC_CRYPTO_SUCCESS
        || tc_sha256_final(signed_digest, &state) != TC_CRYPTO_SUCCESS
        || uECC_verify(public_key, signed_digest, sizeof(signed_digest), header + 64U,
                       uECC_secp256r1())
            != 1) {
        return WATCH_OTA_PACKAGE_RESULT_SIGNATURE;
    }

    info->board_id = read_u32(header + 8U);
    info->firmware_version = read_u32(header + 12U);
    info->security_counter = read_u32(header + 16U);
    info->load_address = read_u32(header + 20U);
    info->image_length = image_length;
    info->key_id = header[60];
    memcpy(info->digest, digest, sizeof(info->digest));
    return WATCH_OTA_PACKAGE_RESULT_OK;
}

const char *watch_ota_package_result_name(watch_ota_package_result_t result)
{
    switch (result) {
    case WATCH_OTA_PACKAGE_RESULT_OK:
        return "ok";
    case WATCH_OTA_PACKAGE_RESULT_INVALID_ARGUMENT:
        return "argument";
    case WATCH_OTA_PACKAGE_RESULT_IO:
        return "io";
    case WATCH_OTA_PACKAGE_RESULT_SIZE:
        return "size";
    case WATCH_OTA_PACKAGE_RESULT_FORMAT:
        return "format";
    case WATCH_OTA_PACKAGE_RESULT_BOARD:
        return "board";
    case WATCH_OTA_PACKAGE_RESULT_RANGE:
        return "range";
    case WATCH_OTA_PACKAGE_RESULT_PADDING:
        return "padding";
    case WATCH_OTA_PACKAGE_RESULT_HASH:
        return "hash";
    case WATCH_OTA_PACKAGE_RESULT_SIGNATURE:
        return "signature";
    case WATCH_OTA_PACKAGE_RESULT_SECURITY_REJECTED:
        return "security";
    case WATCH_OTA_PACKAGE_RESULT_COUNT:
        return "invalid";
    }
    return "invalid";
}
