#include "watch_max30102.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    uint8_t registers[256];
    uint8_t fifo_data[6];
    uint8_t reset_reads_remaining;
    uint8_t write_registers[32];
    uint8_t write_values[32];
    uint8_t write_count;
    uint8_t fifo_write_pointer;
    uint8_t fifo_read_pointer;
    bool fail_read;
} fake_max30102_bus_t;

typedef struct
{
    uint32_t count;
    bool reject;
} fake_publisher_t;

static bool fake_read(void *context, uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    fake_max30102_bus_t *bus = (fake_max30102_bus_t *)context;

    assert(address == WATCH_MAX30102_I2C_ADDRESS);
    if (bus->fail_read) {
        return false;
    }
    if (reg == WATCH_MAX30102_REG_FIFO_WR_PTR) {
        data[0] = bus->fifo_write_pointer;
        return true;
    }
    if (reg == WATCH_MAX30102_REG_FIFO_RD_PTR) {
        data[0] = bus->fifo_read_pointer;
        return true;
    }
    if (reg == WATCH_MAX30102_REG_FIFO_DATA) {
        memcpy(data, bus->fifo_data, length);
        return true;
    }
    if (reg == WATCH_MAX30102_REG_MODE_CONFIG && bus->reset_reads_remaining > 0U) {
        data[0] = WATCH_MAX30102_MODE_RESET;
        bus->reset_reads_remaining--;
        if (bus->reset_reads_remaining == 0U) {
            bus->registers[reg] = 0U;
        }
        return true;
    }

    memcpy(data, &bus->registers[reg], length);
    return true;
}

static bool fake_write(void *context, uint8_t address, uint8_t reg, const uint8_t *data,
                       uint8_t length)
{
    fake_max30102_bus_t *bus = (fake_max30102_bus_t *)context;

    assert(address == WATCH_MAX30102_I2C_ADDRESS);
    assert(length == 1U);
    bus->registers[reg] = data[0];
    if (reg == WATCH_MAX30102_REG_MODE_CONFIG && data[0] == WATCH_MAX30102_MODE_RESET
        && bus->reset_reads_remaining == 0U) {
        bus->registers[reg] = 0U;
    }
    if (bus->write_count < sizeof(bus->write_registers)) {
        bus->write_registers[bus->write_count] = reg;
        bus->write_values[bus->write_count] = data[0];
        bus->write_count++;
    }
    return true;
}

static bool fake_publish(void *context, uint32_t timestamp_ms)
{
    fake_publisher_t *publisher = (fake_publisher_t *)context;

    (void)timestamp_ms;
    publisher->count++;
    return !publisher->reject;
}

static watch_max30102_bus_t fake_bus(fake_max30102_bus_t *bus)
{
    return (watch_max30102_bus_t) {
        .read = fake_read,
        .write = fake_write,
        .context = bus,
    };
}

static void prepare_bus(fake_max30102_bus_t *bus)
{
    memset(bus, 0, sizeof(*bus));
    bus->registers[WATCH_MAX30102_REG_PART_ID] = WATCH_MAX30102_PART_ID;
    bus->registers[WATCH_MAX30102_REG_REV_ID] = 0x02U;
    bus->fifo_data[0] = 0x01U;
    bus->fifo_data[1] = 0x23U;
    bus->fifo_data[2] = 0x45U;
    bus->fifo_data[3] = 0x02U;
    bus->fifo_data[4] = 0x34U;
    bus->fifo_data[5] = 0x56U;
}

static void bring_to_ready(watch_max30102_service_t *service)
{
    assert(watch_max30102_service_process(service, 0U));
    assert(watch_max30102_service_process(service, 1U));
    for (uint32_t now_ms = 2U; now_ms <= 10U; ++now_ms) {
        assert(watch_max30102_service_process(service, now_ms));
    }
}

static void test_identity_and_decode(void)
{
    watch_max30102_sample_t sample;
    const uint8_t fifo_data[6] = { 0x01U, 0x23U, 0x45U, 0x02U, 0x34U, 0x56U };

    assert(watch_max30102_validate_identity(WATCH_MAX30102_PART_ID));
    assert(!watch_max30102_validate_identity(0x00U));
    assert(watch_max30102_decode_fifo_sample(fifo_data, &sample));
    assert(sample.red_raw == 0x12345U);
    assert(sample.ir_raw == 0x23456U);
    assert(sample.finger_on);
}

static void test_service_configures_and_publishes(void)
{
    fake_max30102_bus_t bus;
    fake_publisher_t publisher = { 0 };
    watch_max30102_service_t service;
    watch_max30102_service_status_t status;
    watch_max30102_sample_t sample;
    const watch_max30102_bus_t bus_api = fake_bus(&bus);

    prepare_bus(&bus);
    assert(watch_max30102_service_init(&service, &bus_api, fake_publish, &publisher));
    bring_to_ready(&service);
    assert(watch_max30102_service_read_status(&service, &status));
    assert(status.ready);
    assert(status.part_id == WATCH_MAX30102_PART_ID);
    assert(status.revision_id == 0x02U);
    assert(status.mode_config == WATCH_MAX30102_MODE_SPO2);
    assert(bus.write_registers[0] == WATCH_MAX30102_REG_MODE_CONFIG);
    assert(bus.write_values[0] == WATCH_MAX30102_MODE_RESET);
    assert(bus.write_registers[bus.write_count - 1U] == WATCH_MAX30102_REG_MODE_CONFIG);
    assert(bus.write_values[bus.write_count - 1U] == WATCH_MAX30102_MODE_SPO2);

    assert(!watch_max30102_service_process(&service, 10U));
    assert(watch_max30102_service_read_status(&service, &status));
    assert(status.no_data_count == 1U);
    bus.fifo_write_pointer = 1U;
    bus.fifo_read_pointer = 0U;
    publisher.reject = true;
    assert(watch_max30102_service_process(&service, 50U));
    assert(watch_max30102_service_read_status(&service, &status));
    assert(status.sample_valid);
    assert(status.sample_count == 1U);
    assert(status.event_drop_count == 1U);
    assert(watch_max30102_service_read_latest(&service, &sample));
    assert(sample.red_raw == 0x12345U);
    assert(sample.ir_raw == 0x23456U);
    assert(sample.finger_on);
}

static void test_service_reports_id_and_reset_failures(void)
{
    fake_max30102_bus_t bus;
    watch_max30102_service_t service;
    watch_max30102_service_status_t status;
    watch_max30102_bus_t bus_api;

    prepare_bus(&bus);
    bus.registers[WATCH_MAX30102_REG_PART_ID] = 0x00U;
    bus_api = fake_bus(&bus);
    assert(watch_max30102_service_init(&service, &bus_api, NULL, NULL));
    assert(!watch_max30102_service_process(&service, 0U));
    assert(watch_max30102_service_read_status(&service, &status));
    assert(status.id_error_count == 1U);
    assert(status.state == WATCH_MAX30102_SERVICE_STATE_FAILED);

    prepare_bus(&bus);
    bus.reset_reads_remaining = 10U;
    bus_api = fake_bus(&bus);
    assert(watch_max30102_service_init(&service, &bus_api, NULL, NULL));
    assert(watch_max30102_service_process(&service, 0U));
    assert(!watch_max30102_service_process(&service, 100U));
    assert(watch_max30102_service_read_status(&service, &status));
    assert(status.reset_timeout_count == 1U);
    assert(status.state == WATCH_MAX30102_SERVICE_STATE_FAILED);
}

int main(void)
{
    test_identity_and_decode();
    test_service_configures_and_publishes();
    test_service_reports_id_and_reset_failures();
    return 0;
}
