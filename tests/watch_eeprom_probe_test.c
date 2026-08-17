#include "watch_eeprom_probe.h"

#include <assert.h>
#include <stdint.h>

typedef struct
{
    uint8_t ready_mask;
    uint8_t calls;
    uint8_t addresses[WATCH_EEPROM_PROBE_ADDRESS_COUNT];
} fake_bus_t;

static bool fake_is_ready(void *context, uint8_t address)
{
    fake_bus_t *bus = (fake_bus_t *)context;
    uint8_t index = (uint8_t)(address - WATCH_EEPROM_PROBE_FIRST_ADDRESS);

    bus->addresses[bus->calls] = address;
    bus->calls++;
    return (bus->ready_mask & (uint8_t)(1U << index)) != 0U;
}

static void test_scan_records_only_ready_addresses(void)
{
    fake_bus_t bus = { .ready_mask = 0x05U };
    watch_eeprom_probe_service_t service;
    watch_eeprom_probe_status_t status;

    assert(watch_eeprom_probe_init(&service, fake_is_ready, &bus));
    for (uint8_t index = 0U; index < WATCH_EEPROM_PROBE_ADDRESS_COUNT; index++) {
        assert(watch_eeprom_probe_process(&service));
    }

    assert(!watch_eeprom_probe_process(&service));
    assert(watch_eeprom_probe_read_status(&service, &status));
    assert(status.complete);
    assert(status.first_address == 0x50U);
    assert(status.last_address == 0x57U);
    assert(status.probed_count == WATCH_EEPROM_PROBE_ADDRESS_COUNT);
    assert(status.response_mask == 0x05U);
    assert(status.scan_count == 1U);
    assert(bus.calls == WATCH_EEPROM_PROBE_ADDRESS_COUNT);
    assert(bus.addresses[0] == 0x50U);
    assert(bus.addresses[7] == 0x57U);
}

static void test_invalid_arguments_are_rejected(void)
{
    watch_eeprom_probe_service_t service;
    watch_eeprom_probe_status_t status;

    assert(!watch_eeprom_probe_init(NULL, fake_is_ready, NULL));
    assert(!watch_eeprom_probe_init(&service, NULL, NULL));
    assert(!watch_eeprom_probe_process(NULL));
    assert(!watch_eeprom_probe_read_status(NULL, &status));
    assert(!watch_eeprom_probe_read_status(&service, NULL));
}

int main(void)
{
    test_scan_records_only_ready_addresses();
    test_invalid_arguments_are_rejected();
    return 0;
}
