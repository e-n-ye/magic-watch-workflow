#include "watch_resource_protocol.h"

#include <assert.h>
#include <string.h>

#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>

#define TEST_FILE_SIZE 700U
#define RESPONSE_SIZE (WATCH_RESOURCE_PROTOCOL_HEADER_SIZE + 5U)

typedef struct
{
    uint8_t data[TEST_FILE_SIZE];
    size_t size;
    bool active;
    bool fail_data;
    unsigned int begin_count;
    unsigned int commit_count;
    unsigned int abort_count;
    uint8_t response[RESPONSE_SIZE];
    size_t response_length;
} test_context_t;

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length)
{
    for (size_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xEDB88320UL : crc >> 1U;
        }
    }
    return crc;
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

static uint32_t frame_crc(const uint8_t *frame, size_t payload_length)
{
    uint32_t crc = crc32_update(0xFFFFFFFFUL, frame, 14U);
    crc = crc32_update(crc, frame + WATCH_RESOURCE_PROTOCOL_HEADER_SIZE, payload_length);
    return crc ^ 0xFFFFFFFFUL;
}

static size_t make_frame(uint8_t *frame, uint8_t type, uint32_t sequence,
                         const uint8_t *payload, size_t payload_length)
{
    memcpy(frame, "MWRP", 4U);
    frame[4] = WATCH_RESOURCE_PROTOCOL_VERSION;
    frame[5] = type;
    write_u16(frame + 6U, 0U);
    write_u32(frame + 8U, sequence);
    write_u16(frame + 12U, (uint16_t)payload_length);
    if (payload_length > 0U) {
        memcpy(frame + WATCH_RESOURCE_PROTOCOL_HEADER_SIZE, payload, payload_length);
    }
    write_u32(frame + 14U, frame_crc(frame, payload_length));
    return WATCH_RESOURCE_PROTOCOL_HEADER_SIZE + payload_length;
}

static bool sink_begin(void *context, const char *path, uint32_t size,
                       const uint8_t digest[WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE])
{
    test_context_t *test = context;
    assert(strcmp(path, "/asset.bin") == 0);
    assert(size == TEST_FILE_SIZE || size == 1U || size == 0U);
    (void)digest;
    test->active = true;
    test->size = 0U;
    ++test->begin_count;
    return true;
}

static bool sink_data(void *context, uint32_t offset, const uint8_t *data, size_t length)
{
    test_context_t *test = context;

    if (test->fail_data || !test->active || offset != test->size || length > sizeof(test->data) - offset) {
        return false;
    }
    memcpy(&test->data[offset], data, length);
    test->size += length;
    return true;
}

static bool sink_commit(void *context)
{
    test_context_t *test = context;
    assert(test->active);
    test->active = false;
    ++test->commit_count;
    return true;
}

static void sink_abort(void *context)
{
    test_context_t *test = context;
    test->active = false;
    ++test->abort_count;
}

static bool emit_response(void *context, const uint8_t *data, size_t length)
{
    test_context_t *test = context;
    assert(length == RESPONSE_SIZE);
    memcpy(test->response, data, length);
    test->response_length = length;
    return true;
}

static uint8_t response_error(const test_context_t *test)
{
    assert(test->response_length == RESPONSE_SIZE);
    return test->response[WATCH_RESOURCE_PROTOCOL_HEADER_SIZE];
}

static uint8_t response_type(const test_context_t *test)
{
    assert(test->response_length == RESPONSE_SIZE);
    return test->response[5];
}

static void fill_digest(uint8_t digest[WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE], const uint8_t *data,
                        size_t length)
{
    struct tc_sha256_state_struct state;
    assert(tc_sha256_init(&state) == TC_CRYPTO_SUCCESS);
    assert(tc_sha256_update(&state, data, length) == TC_CRYPTO_SUCCESS);
    assert(tc_sha256_final(digest, &state) == TC_CRYPTO_SUCCESS);
}

static void send_begin(watch_resource_protocol_t *protocol, test_context_t *test,
                       const uint8_t *data, size_t length, uint32_t sequence)
{
    uint8_t payload[2U + 10U + 4U + WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE];
    uint8_t digest[WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE];
    uint8_t frame[WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_length;

    fill_digest(digest, data, length);
    write_u16(payload, 10U);
    memcpy(payload + 2U, "/asset.bin", 10U);
    write_u32(payload + 12U, (uint32_t)length);
    memcpy(payload + 16U, digest, sizeof(digest));
    frame_length = make_frame(frame, WATCH_RESOURCE_FRAME_BEGIN, sequence, payload, sizeof(payload));
    assert(watch_resource_protocol_feed(protocol, frame, frame_length) == frame_length);
    assert(response_type(test) == WATCH_RESOURCE_FRAME_ACK);
    assert(response_error(test) == WATCH_RESOURCE_ERROR_NONE);
}

static void test_valid_transfer(void)
{
    test_context_t test = { 0 };
    watch_resource_protocol_t protocol;
    watch_resource_sink_t sink = {
        .begin = sink_begin,
        .data = sink_data,
        .commit = sink_commit,
        .abort = sink_abort,
        .context = &test,
    };
    uint8_t source[TEST_FILE_SIZE];
    uint8_t payload[WATCH_RESOURCE_PROTOCOL_MAX_PAYLOAD];
    uint8_t frame[WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_length;

    for (size_t index = 0U; index < sizeof(source); ++index) {
        source[index] = (uint8_t)(index * 17U + 3U);
    }
    watch_resource_protocol_init(&protocol, &sink, emit_response, &test);
    send_begin(&protocol, &test, source, sizeof(source), 0U);

    write_u32(payload, 0U);
    memcpy(payload + 4U, source, 508U);
    frame_length = make_frame(frame, WATCH_RESOURCE_FRAME_DATA, 1U, payload, 512U);
    assert(watch_resource_protocol_feed(&protocol, frame, 7U) == 7U);
    assert(watch_resource_protocol_feed(&protocol, frame + 7U, frame_length - 7U) == frame_length - 7U);
    assert(response_type(&test) == WATCH_RESOURCE_FRAME_ACK);
    write_u32(payload, 508U);
    memcpy(payload + 4U, source + 508U, sizeof(source) - 508U);
    frame_length = make_frame(frame, WATCH_RESOURCE_FRAME_DATA, 2U, payload, 196U);
    assert(watch_resource_protocol_feed(&protocol, frame, frame_length) == frame_length);
    assert(response_type(&test) == WATCH_RESOURCE_FRAME_ACK);
    frame_length = make_frame(frame, WATCH_RESOURCE_FRAME_COMMIT, 3U, NULL, 0U);
    assert(watch_resource_protocol_feed(&protocol, frame, frame_length) == frame_length);
    assert(response_type(&test) == WATCH_RESOURCE_FRAME_ACK);
    assert(test.begin_count == 1U && test.commit_count == 1U && test.abort_count == 0U);
    assert(test.size == sizeof(source) && memcmp(test.data, source, sizeof(source)) == 0);
    assert(!watch_resource_protocol_is_active(&protocol));
}

static void test_rejections(void)
{
    test_context_t test = { 0 };
    watch_resource_protocol_t protocol;
    watch_resource_sink_t sink = {
        .begin = sink_begin,
        .data = sink_data,
        .commit = sink_commit,
        .abort = sink_abort,
        .context = &test,
    };
    uint8_t frame[WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE];
    uint8_t payload[64] = { 0 };
    size_t length;

    length = make_frame(frame, WATCH_RESOURCE_FRAME_BEGIN, 0U, payload, sizeof(payload));
    frame[4] = WATCH_RESOURCE_PROTOCOL_VERSION + 1U;
    write_u32(frame + 14U, frame_crc(frame, sizeof(payload)));
    watch_resource_protocol_init(&protocol, &sink, emit_response, &test);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_VERSION);

    length = make_frame(frame, WATCH_RESOURCE_FRAME_BEGIN, 0U, payload, sizeof(payload));
    frame[6] = 1U;
    write_u32(frame + 14U, frame_crc(frame, sizeof(payload)));
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_FLAGS);

    memcpy(frame, "MWRP", 4U);
    frame[4] = WATCH_RESOURCE_PROTOCOL_VERSION;
    frame[5] = WATCH_RESOURCE_FRAME_BEGIN;
    write_u16(frame + 6U, 0U);
    write_u32(frame + 8U, 0U);
    write_u16(frame + 12U, WATCH_RESOURCE_PROTOCOL_MAX_PAYLOAD + 1U);
    length = WATCH_RESOURCE_PROTOCOL_HEADER_SIZE;
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_LENGTH);

    watch_resource_protocol_init(&protocol, &sink, emit_response, &test);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_BEGIN, 0U, (const uint8_t *)"bad", 3U);
    frame[length - 1U] ^= 0x01U;
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_type(&test) == WATCH_RESOURCE_FRAME_NACK);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_CRC);

    write_u16(payload, 4U);
    memcpy(payload + 2U, "/../", 4U);
    write_u32(payload + 6U, 0U);
    memset(payload + 10U, 0, WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_BEGIN, 0U, payload,
                        2U + 4U + 4U + WATCH_RESOURCE_PROTOCOL_DIGEST_SIZE);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_BEGIN);

    write_u32(payload, 99U);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_DATA, 1U, payload, 4U);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_STATE);

    assert(!watch_resource_protocol_is_active(&protocol));
}

static void test_digest_and_abort(void)
{
    test_context_t test = { 0 };
    watch_resource_protocol_t protocol;
    watch_resource_sink_t sink = {
        .begin = sink_begin,
        .data = sink_data,
        .commit = sink_commit,
        .abort = sink_abort,
        .context = &test,
    };
    uint8_t source[TEST_FILE_SIZE] = { 0x55 };
    uint8_t payload[WATCH_RESOURCE_PROTOCOL_MAX_PAYLOAD] = { 0 };
    uint8_t frame[WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE];
    size_t length;

    watch_resource_protocol_init(&protocol, &sink, emit_response, &test);
    send_begin(&protocol, &test, source, sizeof(source), 0U);
    write_u32(payload, 0U);
    memcpy(payload + 4U, source, 10U);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_DATA, 2U, payload, 14U);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_SEQUENCE);
    write_u32(payload, 1U);
    memcpy(payload + 4U, source, 10U);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_DATA, 1U, payload, 14U);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_OFFSET);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_ABORT, 1U, NULL, 0U);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_type(&test) == WATCH_RESOURCE_FRAME_ACK);
    assert(test.abort_count == 1U && !watch_resource_protocol_is_active(&protocol));
}

static void test_zero_length_and_sink_failure(void)
{
    test_context_t test = { 0 };
    watch_resource_protocol_t protocol;
    watch_resource_sink_t sink = {
        .begin = sink_begin,
        .data = sink_data,
        .commit = sink_commit,
        .abort = sink_abort,
        .context = &test,
    };
    uint8_t empty[1] = { 0 };
    uint8_t one[1] = { 0x55 };
    uint8_t failed_payload[5] = { 0 };
    uint8_t frame[WATCH_RESOURCE_PROTOCOL_MAX_FRAME_SIZE];
    size_t length;

    watch_resource_protocol_init(&protocol, &sink, emit_response, &test);
    send_begin(&protocol, &test, empty, 0U, 0U);
    length = make_frame(frame, WATCH_RESOURCE_FRAME_COMMIT, 1U, NULL, 0U);
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_type(&test) == WATCH_RESOURCE_FRAME_ACK);
    assert(test.commit_count == 1U);

    memset(&test, 0, sizeof(test));
    watch_resource_protocol_init(&protocol, &sink, emit_response, &test);
    send_begin(&protocol, &test, one, sizeof(one), 0U);
    test.fail_data = true;
    write_u32(failed_payload, 0U);
    failed_payload[4] = (uint8_t)'x';
    length = make_frame(frame, WATCH_RESOURCE_FRAME_DATA, 1U, failed_payload,
                        sizeof(failed_payload));
    watch_resource_protocol_feed(&protocol, frame, length);
    assert(response_error(&test) == WATCH_RESOURCE_ERROR_STORAGE);
    assert(test.abort_count == 1U);
}

int main(void)
{
    test_valid_transfer();
    test_rejections();
    test_digest_and_abort();
    test_zero_length_and_sink_failure();
    return 0;
}
