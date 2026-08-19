#include "manifest.h"

#include <stddef.h>
#include <stdint.h>

#include "stm32f411xe.h"
#include "tinycrypt/constants.h"
#include "tinycrypt/ecc.h"
#include "tinycrypt/ecc_dsa.h"
#include "tinycrypt/sha256.h"
#include "tinycrypt/utils.h"

#define APPLICATION_FLASH_START 0x08010000UL
#define APPLICATION_EXECUTION_END 0x0807F000UL
#define APPLICATION_TRAILER_START APPLICATION_EXECUTION_END
#define APPLICATION_TRAILER_SIZE 0x1000UL
#define APPLICATION_MAX_IMAGE_LENGTH (APPLICATION_EXECUTION_END - APPLICATION_FLASH_START)
#define SRAM_START 0x20000000UL
#define SRAM_END 0x20020000UL

#define MANIFEST_HEADER_SIZE 128U
#define MANIFEST_SIGNED_SIZE 64U
#define MANIFEST_SIGNATURE_SIZE 64U
#define MANIFEST_FORMAT_VERSION 1U
#define MANIFEST_BOARD_ID 0x31313446UL
#define MANIFEST_KEY_ID 0U

static const uint8_t s_bootloader_public_key[64] = {
    0xa2, 0xbe, 0xdb, 0x37, 0x4d, 0x14, 0xf8, 0xae, 0x2c, 0xbb, 0xfe, 0x23, 0x36, 0x4e, 0x36, 0xea,
    0x5a, 0x39, 0x1d, 0xb1, 0x14, 0x72, 0x82, 0x17, 0x25, 0x6b, 0xe3, 0x9f, 0x44, 0x7d, 0x40, 0x69,
    0xf2, 0x0b, 0x59, 0x08, 0xce, 0xa6, 0xdc, 0x76, 0xe7, 0x29, 0x52, 0x3f, 0xce, 0xac, 0x10, 0x5a,
    0xf9, 0x94, 0x5b, 0x55, 0xc0, 0x53, 0x51, 0x73, 0x5a, 0x93, 0x8e, 0x2b, 0xe6, 0xb5, 0xc8, 0xbe,
};

/* Verification never requests randomness; this satisfies TinyCrypt's POSIX
 * default hook without adding a platform-specific entropy implementation. */
int default_CSPRNG(uint8_t *destination, unsigned int size)
{
    (void)destination;
    (void)size;
    return 0;
}

static uint16_t manifest_read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t manifest_read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) | ((uint32_t)data[2] << 16U)
        | ((uint32_t)data[3] << 24U);
}

static int manifest_hash(const uint8_t *data, size_t length, uint8_t digest[32])
{
    struct tc_sha256_state_struct state;

    if (tc_sha256_init(&state) != TC_CRYPTO_SUCCESS
        || tc_sha256_update(&state, data, length) != TC_CRYPTO_SUCCESS
        || tc_sha256_final(digest, &state) != TC_CRYPTO_SUCCESS) {
        return 0;
    }

    return 1;
}

static int manifest_padding_is_erased(const uint8_t *trailer)
{
    for (uint32_t offset = MANIFEST_HEADER_SIZE; offset < APPLICATION_TRAILER_SIZE; offset++) {
        if (trailer[offset] != 0xFFU) {
            return 0;
        }
    }

    return 1;
}

static int application_padding_is_erased(uint32_t image_length)
{
    const uint8_t *padding = (const uint8_t *)(APPLICATION_FLASH_START + image_length);

    for (uint32_t offset = image_length; offset < APPLICATION_MAX_IMAGE_LENGTH; offset++) {
        if (padding[offset - image_length] != 0xFFU) {
            return 0;
        }
    }

    return 1;
}

static int application_vector_is_valid(uint32_t image_length)
{
    volatile const uint32_t *vector_table = (volatile const uint32_t *)APPLICATION_FLASH_START;
    uint32_t main_stack_pointer = vector_table[0];
    uint32_t reset_vector = vector_table[1];
    uint32_t reset_address = reset_vector & ~1UL;

    if ((main_stack_pointer & 0x7UL) != 0UL || main_stack_pointer < SRAM_START
        || main_stack_pointer > SRAM_END) {
        return 0;
    }

    if ((reset_vector & 1UL) == 0UL || reset_address < APPLICATION_FLASH_START
        || reset_address >= APPLICATION_FLASH_START + image_length) {
        return 0;
    }

    return 1;
}

int bootloader_application_is_valid(void)
{
    const uint8_t *trailer = (const uint8_t *)APPLICATION_TRAILER_START;
    const uint8_t *signature = trailer + MANIFEST_SIGNED_SIZE;
    uint8_t image_digest[32];
    uint8_t signed_header_digest[32];
    uint32_t image_length;

    if (trailer[0] != 'M' || trailer[1] != 'W' || trailer[2] != 'M' || trailer[3] != 'F'
        || manifest_read_u16(trailer + 4U) != MANIFEST_FORMAT_VERSION
        || manifest_read_u16(trailer + 6U) != MANIFEST_HEADER_SIZE
        || manifest_read_u32(trailer + 8U) != MANIFEST_BOARD_ID
        || manifest_read_u32(trailer + 20U) != APPLICATION_FLASH_START
        || trailer[60] != MANIFEST_KEY_ID || trailer[61] != 0U || trailer[62] != 0U
        || trailer[63] != 0U) {
        return 0;
    }

    image_length = manifest_read_u32(trailer + 24U);
    if (image_length == 0UL || image_length > APPLICATION_MAX_IMAGE_LENGTH
        || !manifest_padding_is_erased(trailer) || !application_padding_is_erased(image_length)
        || !application_vector_is_valid(image_length)) {
        return 0;
    }

    if (!manifest_hash((const uint8_t *)APPLICATION_FLASH_START, image_length, image_digest)
        || _compare(image_digest, trailer + 28U, sizeof(image_digest)) != 0
        || !manifest_hash(trailer, MANIFEST_SIGNED_SIZE, signed_header_digest)
        || uECC_verify(s_bootloader_public_key, signed_header_digest, sizeof(signed_header_digest),
                       signature, uECC_secp256r1())
            != 1) {
        return 0;
    }

    return 1;
}

const uint8_t *bootloader_public_key(void)
{
    return s_bootloader_public_key;
}
