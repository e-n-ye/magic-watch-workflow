#include "watch_ota_package.h"
#include "watch_ymodem.h"

#include <assert.h>
#include <string.h>

#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>

typedef struct
{
    uint8_t data[256];
    size_t size;
    unsigned int begin_count;
    unsigned int commit_count;
    unsigned int abort_count;
    uint8_t emitted[32];
    size_t emitted_length;
} ymodem_sink_test_t;

static bool emit_byte(void *context, uint8_t byte)
{
    ymodem_sink_test_t *test = context;
    assert(test->emitted_length < sizeof(test->emitted));
    test->emitted[test->emitted_length++] = byte;
    return true;
}

static bool sink_begin(void *context, const char *name, uint32_t size)
{
    ymodem_sink_test_t *test = context;
    assert(strcmp(name, "candidate.bin") == 0 && size == 200U);
    test->size = 0U;
    ++test->begin_count;
    return true;
}

static bool sink_data(void *context, uint32_t offset, const uint8_t *data, size_t length)
{
    ymodem_sink_test_t *test = context;
    assert(offset == test->size && length <= sizeof(test->data) - offset);
    memcpy(&test->data[offset], data, length);
    test->size += length;
    return true;
}

static bool sink_commit(void *context)
{
    ymodem_sink_test_t *test = context;
    ++test->commit_count;
    return true;
}

static void sink_abort(void *context)
{
    ++((ymodem_sink_test_t *)context)->abort_count;
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0U;
    for (size_t index = 0U; index < length; ++index) {
        crc ^= (uint16_t)data[index] << 8U;
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x8000U) != 0U ? (uint16_t)((crc << 1U) ^ 0x1021U)
                                        : (uint16_t)(crc << 1U);
        }
    }
    return crc;
}

static size_t make_packet(uint8_t *packet, uint8_t start, uint8_t block, const uint8_t *data,
                          size_t data_length)
{
    size_t block_size = start == WATCH_YMODEM_STX ? 1024U : 128U;
    uint16_t checksum;

    memset(packet, 0x1A, block_size + 5U);
    packet[0] = start;
    packet[1] = block;
    packet[2] = (uint8_t)~block;
    memcpy(&packet[3], data, data_length);
    checksum = crc16(&packet[3], block_size);
    packet[block_size + 3U] = (uint8_t)(checksum >> 8U);
    packet[block_size + 4U] = (uint8_t)checksum;
    return block_size + 5U;
}

static void test_ymodem_transfer(void)
{
    ymodem_sink_test_t sink_context = { 0 };
    watch_ymodem_sink_t sink = {
        .begin = sink_begin,
        .data = sink_data,
        .commit = sink_commit,
        .abort = sink_abort,
        .context = &sink_context,
    };
    watch_ymodem_t protocol;
    uint8_t header_data[128] = { 0 };
    uint8_t source[200];
    uint8_t packet[WATCH_YMODEM_MAX_PACKET_SIZE];
    size_t packet_length;

    memcpy(header_data, "candidate.bin", sizeof("candidate.bin") - 1U);
    memcpy(header_data + sizeof("candidate.bin"), "200", 4U);
    for (size_t index = 0U; index < sizeof(source); ++index) {
        source[index] = (uint8_t)(index * 3U + 1U);
    }
    assert(watch_ymodem_init(&protocol, &sink, emit_byte, &sink_context));
    assert(watch_ymodem_start(&protocol) == WATCH_YMODEM_RESULT_OK);
    packet_length = make_packet(packet, WATCH_YMODEM_SOH, 0U, header_data, sizeof(header_data));
    for (size_t offset = 0U; offset < packet_length; offset += 3U) {
        size_t chunk = packet_length - offset > 3U ? 3U : packet_length - offset;
        assert(watch_ymodem_feed(&protocol, packet + offset, chunk) == chunk);
    }
    packet_length = make_packet(packet, WATCH_YMODEM_SOH, 1U, source, 128U);
    assert(watch_ymodem_feed(&protocol, packet, packet_length) == packet_length);
    assert(watch_ymodem_feed(&protocol, packet, packet_length) == packet_length);
    packet_length = make_packet(packet, WATCH_YMODEM_SOH, 2U, source + 128U, 72U);
    assert(watch_ymodem_feed(&protocol, packet, packet_length) == packet_length);
    packet[packet_length - 1U] ^= 1U;
    assert(watch_ymodem_feed(&protocol, packet, packet_length) == packet_length);
    assert(watch_ymodem_result(&protocol) == WATCH_YMODEM_RESULT_CRC);
    packet[packet_length - 1U] ^= 1U;
    assert(watch_ymodem_feed(&protocol, packet, packet_length) == packet_length);
    assert(watch_ymodem_feed(&protocol, (const uint8_t[]){ WATCH_YMODEM_EOT }, 1U) == 1U);
    assert(watch_ymodem_state(&protocol) == WATCH_YMODEM_STATE_COMPLETE);
    assert(sink_context.size == sizeof(source) && memcmp(sink_context.data, source, sizeof(source)) == 0);
    assert(sink_context.begin_count == 1U && sink_context.commit_count == 1U
           && sink_context.abort_count == 0U);
    assert(sink_context.emitted[0] == WATCH_YMODEM_CRC_REQUEST);
}

static void test_ymodem_rejections(void)
{
    ymodem_sink_test_t sink_context = { 0 };
    watch_ymodem_sink_t sink = {
        .begin = sink_begin,
        .data = sink_data,
        .commit = sink_commit,
        .abort = sink_abort,
        .context = &sink_context,
    };
    watch_ymodem_t protocol;
    uint8_t packet[WATCH_YMODEM_MAX_PACKET_SIZE] = { 0 };

    assert(watch_ymodem_init(&protocol, &sink, emit_byte, &sink_context));
    assert(watch_ymodem_start(&protocol) == WATCH_YMODEM_RESULT_OK);
    packet[0] = WATCH_YMODEM_SOH;
    packet[1] = 0U;
    packet[2] = 0xFFU;
    memset(packet + 3U, 0, 128U);
    packet[3U + 128U] = 0U;
    packet[4U + 128U] = 0U;
    assert(watch_ymodem_feed(&protocol, packet, 133U) == 133U);
    assert(watch_ymodem_state(&protocol) == WATCH_YMODEM_STATE_ERROR);
    assert(sink_context.abort_count == 0U);
}

typedef struct
{
    uint8_t data[WATCH_OTA_PACKAGE_SIZE];
} package_test_t;

static bool package_read(void *context, uint32_t offset, uint8_t *data, size_t length)
{
    package_test_t *package = context;
    if ((uint64_t)offset + length > sizeof(package->data)) {
        return false;
    }
    memcpy(data, &package->data[offset], length);
    return true;
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

static void make_package(package_test_t *package)
{
    uint8_t digest[32];
    struct tc_sha256_state_struct state;

    memset(package->data, 0xFF, sizeof(package->data));
    for (size_t index = 0U; index < 16U; ++index) {
        package->data[index] = (uint8_t)(index + 1U);
    }
    assert(tc_sha256_init(&state) == TC_CRYPTO_SUCCESS);
    assert(tc_sha256_update(&state, package->data, 16U) == TC_CRYPTO_SUCCESS);
    assert(tc_sha256_final(digest, &state) == TC_CRYPTO_SUCCESS);
    memcpy(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET, "MWMF", 4U);
    write_u16(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 4U, 1U);
    write_u16(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 6U, 128U);
    write_u32(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 8U, WATCH_OTA_PACKAGE_BOARD_ID);
    write_u32(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 12U, 7U);
    write_u32(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 16U, 3U);
    write_u32(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 20U, WATCH_OTA_PACKAGE_LOAD_ADDRESS);
    write_u32(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 24U, 16U);
    memcpy(package->data + WATCH_OTA_PACKAGE_TRAILER_OFFSET + 28U, digest, sizeof(digest));
    package->data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 60U] = 0U;
    package->data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 61U] = 0U;
    package->data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 62U] = 0U;
    package->data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 63U] = 0U;
    package->data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 64U] = 1U;
}

static void test_package_rejections(void)
{
    static const uint8_t public_key[64] = {
        0xa2, 0xbe, 0xdb, 0x37, 0x4d, 0x14, 0xf8, 0xae, 0x2c, 0xbb, 0xfe, 0x23,
        0x36, 0x4e, 0x36, 0xea, 0x5a, 0x39, 0x1d, 0xb1, 0x14, 0x72, 0x82, 0x17,
        0x25, 0x6b, 0xe3, 0x9f, 0x44, 0x7d, 0x40, 0x69, 0xf2, 0x0b, 0x59, 0x08,
        0xce, 0xa6, 0xdc, 0x76, 0xe7, 0x29, 0x52, 0x3f, 0xce, 0xac, 0x10, 0x5a,
        0xf9, 0x94, 0x5b, 0x55, 0xc0, 0x53, 0x51, 0x73, 0x5a, 0x93, 0x8e, 0x2b,
        0xe6, 0xb5, 0xc8, 0xbe,
    };
    package_test_t package;
    watch_ota_package_reader_t reader;
    watch_ota_package_info_t info;

    make_package(&package);
    reader = (watch_ota_package_reader_t) {
        .read = package_read,
        .context = &package,
        .size = WATCH_OTA_PACKAGE_SIZE,
    };
    assert(watch_ota_package_verify(&reader, WATCH_OTA_PACKAGE_BOARD_ID, 4U, public_key, &info)
           == WATCH_OTA_PACKAGE_RESULT_SECURITY_REJECTED);
    assert(watch_ota_package_verify(&reader, 0U, 0U, public_key, &info)
           == WATCH_OTA_PACKAGE_RESULT_BOARD);
    package.data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 200U] = 0U;
    assert(watch_ota_package_verify(&reader, WATCH_OTA_PACKAGE_BOARD_ID, 0U, public_key, &info)
           == WATCH_OTA_PACKAGE_RESULT_PADDING);
    package.data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 200U] = 0xFFU;
    package.data[WATCH_OTA_PACKAGE_TRAILER_OFFSET + 28U] ^= 1U;
    assert(watch_ota_package_verify(&reader, WATCH_OTA_PACKAGE_BOARD_ID, 0U, public_key, &info)
           == WATCH_OTA_PACKAGE_RESULT_HASH);
}

int main(void)
{
    test_ymodem_transfer();
    test_ymodem_rejections();
    test_package_rejections();
    assert(strcmp(watch_ymodem_result_name(WATCH_YMODEM_RESULT_CRC), "crc") == 0);
    assert(strcmp(watch_ota_package_result_name(WATCH_OTA_PACKAGE_RESULT_SIGNATURE), "signature")
           == 0);
    return 0;
}
