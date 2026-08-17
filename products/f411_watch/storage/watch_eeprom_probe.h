#ifndef WATCH_EEPROM_PROBE_H
#define WATCH_EEPROM_PROBE_H

#include <stdbool.h>
#include <stdint.h>

#define WATCH_EEPROM_PROBE_FIRST_ADDRESS 0x50U
#define WATCH_EEPROM_PROBE_LAST_ADDRESS 0x57U
#define WATCH_EEPROM_PROBE_ADDRESS_COUNT                                                           \
    (WATCH_EEPROM_PROBE_LAST_ADDRESS - WATCH_EEPROM_PROBE_FIRST_ADDRESS + 1U)

typedef bool (*watch_eeprom_probe_is_ready_fn)(void *context, uint8_t address);

typedef struct
{
    watch_eeprom_probe_is_ready_fn is_ready;
    void *context;
    uint8_t next_index;
    uint8_t probed_count;
    uint8_t response_mask;
    uint32_t scan_count;
    bool complete;
    bool initialized;
} watch_eeprom_probe_service_t;

typedef struct
{
    bool complete;
    uint8_t first_address;
    uint8_t last_address;
    uint8_t probed_count;
    uint8_t response_mask;
    uint32_t scan_count;
} watch_eeprom_probe_status_t;

bool watch_eeprom_probe_init(watch_eeprom_probe_service_t *service,
                             watch_eeprom_probe_is_ready_fn is_ready, void *context);
bool watch_eeprom_probe_process(watch_eeprom_probe_service_t *service);
bool watch_eeprom_probe_read_status(const watch_eeprom_probe_service_t *service,
                                    watch_eeprom_probe_status_t *status);

#endif /* WATCH_EEPROM_PROBE_H */
