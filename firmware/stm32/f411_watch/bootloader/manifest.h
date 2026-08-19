#ifndef F411_BOOTLOADER_MANIFEST_H
#define F411_BOOTLOADER_MANIFEST_H

#include <stdint.h>

int bootloader_application_is_valid(void);
const uint8_t *bootloader_public_key(void);

#endif
