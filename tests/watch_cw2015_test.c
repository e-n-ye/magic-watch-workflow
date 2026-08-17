#include "watch_cw2015.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    uint8_t registers[256];
    bool fail_read;
} fake_cw2015_bus_t;

typedef struct
{
    uint32_t count;
    bool reject;
} fake_publisher_t;

static bool fake_read(void *context, uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    fake_cw2015_bus_t *bus = (fake_cw2015_bus_t *)context;

    assert(address == WATCH_CW2015_I2C_ADDRESS);
    if (bus->fail_read) {
        return false;
    }
    memcpy(data, &bus->registers[reg], length);
    return true;
}

static bool fake_publish(void *context, uint32_t timestamp_ms)
{
    fake_publisher_t *publisher = (fake_publisher_t *)context;

    (void)timestamp_ms;
    publisher->count++;
    return !publisher->reject;
}

static void prepare_bus(fake_cw2015_bus_t *bus)
{
    memset(bus, 0, sizeof(*bus));
    bus->registers[WATCH_CW2015_REG_VERSION] = 0x12U;
    bus->registers[WATCH_CW2015_REG_VCELL] = 0x12U;
    bus->registers[WATCH_CW2015_REG_VCELL + 1U] = 0x34U;
    bus->registers[WATCH_CW2015_REG_SOC] = 42U;
    bus->registers[WATCH_CW2015_REG_SOC + 1U] = 128U;
}

static void test_decode(void)
{
    const uint8_t voltage_data[2] = { 0x12U, 0x34U };
    const uint8_t soc_data[2] = { 42U, 128U };
    const uint8_t invalid_soc[2] = { WATCH_CW2015_SOC_INVALID, 0U };
    uint16_t voltage_mv;
    uint8_t soc_percent;
    uint8_t soc_fraction;

    assert(watch_cw2015_decode_voltage(voltage_data, &voltage_mv));
    assert(voltage_mv == 1421U);
    assert(watch_cw2015_decode_soc(soc_data, &soc_percent, &soc_fraction));
    assert(soc_percent == 42U);
    assert(soc_fraction == 128U);
    assert(!watch_cw2015_decode_soc(invalid_soc, &soc_percent, &soc_fraction));
}

static void test_service_reads_and_publishes(void)
{
    fake_cw2015_bus_t bus;
    fake_publisher_t publisher = { 0 };
    watch_cw2015_service_t service;
    watch_cw2015_service_status_t status;
    watch_cw2015_sample_t sample;
    const watch_cw2015_bus_t bus_api = { .read = fake_read, .context = &bus };

    prepare_bus(&bus);
    assert(watch_cw2015_service_init(&service, &bus_api, fake_publish, &publisher));
    assert(watch_cw2015_service_process(&service, 0U));
    assert(watch_cw2015_service_read_status(&service, &status));
    assert(status.ready);
    assert(status.version == 0x12U);
    assert(status.sample_count == 1U);
    assert(status.sample_valid);
    assert(status.voltage_mv == 1421U);
    assert(status.soc_percent == 42U);
    assert(publisher.count == 1U);
    assert(watch_cw2015_service_read_latest(&service, &sample));
    assert(sample.soc_fraction == 128U);

    assert(!watch_cw2015_service_process(&service, 500U));
    assert(watch_cw2015_service_process(&service, 1000U));
    assert(watch_cw2015_service_read_status(&service, &status));
    assert(status.sample_count == 2U);
}

static void test_service_errors_and_event_drop(void)
{
    fake_cw2015_bus_t bus;
    fake_publisher_t publisher = { .reject = true };
    watch_cw2015_service_t service;
    watch_cw2015_service_status_t status;
    const watch_cw2015_bus_t bus_api = { .read = fake_read, .context = &bus };

    prepare_bus(&bus);
    assert(watch_cw2015_service_init(&service, &bus_api, fake_publish, &publisher));
    assert(watch_cw2015_service_process(&service, 0U));
    assert(watch_cw2015_service_read_status(&service, &status));
    assert(status.event_drop_count == 1U);

    bus.registers[WATCH_CW2015_REG_SOC] = WATCH_CW2015_SOC_INVALID;
    assert(!watch_cw2015_service_process(&service, 1000U));
    assert(watch_cw2015_service_read_status(&service, &status));
    assert(status.invalid_soc_count == 1U);

    bus.fail_read = true;
    assert(!watch_cw2015_service_process(&service, 2000U));
    assert(watch_cw2015_service_read_status(&service, &status));
    assert(status.read_error_count == 1U);
}

int main(void)
{
    test_decode();
    test_service_reads_and_publishes();
    test_service_errors_and_event_drop();
    return 0;
}
