#include "watch_lsm6ds3.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    uint8_t registers[256];
    uint8_t write_registers[3];
    uint8_t write_values[3];
    uint8_t write_count;
    bool read_failed;
    bool write_failed;
} fake_lsm6ds3_bus_t;

typedef struct
{
    uint32_t count;
    uint32_t last_timestamp_ms;
    bool reject;
} fake_publisher_t;

static bool fake_read(void *context, uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    fake_lsm6ds3_bus_t *bus = (fake_lsm6ds3_bus_t *)context;

    assert(address == WATCH_LSM6DS3_I2C_ADDRESS);
    if (bus->read_failed) {
        return false;
    }

    memcpy(data, &bus->registers[reg], length);
    return true;
}

static bool fake_write(void *context, uint8_t address, uint8_t reg, const uint8_t *data,
                       uint8_t length)
{
    fake_lsm6ds3_bus_t *bus = (fake_lsm6ds3_bus_t *)context;

    assert(address == WATCH_LSM6DS3_I2C_ADDRESS);
    assert(length == 1U);
    if (bus->write_failed) {
        return false;
    }

    bus->registers[reg] = data[0];
    if (bus->write_count < 3U) {
        bus->write_registers[bus->write_count] = reg;
        bus->write_values[bus->write_count] = data[0];
        bus->write_count++;
    }
    return true;
}

static watch_lsm6ds3_bus_t fake_bus(fake_lsm6ds3_bus_t *bus)
{
    return (watch_lsm6ds3_bus_t) {
        .read = fake_read,
        .write = fake_write,
        .context = bus,
    };
}

static bool fake_publish(void *context, uint32_t timestamp_ms)
{
    fake_publisher_t *publisher = (fake_publisher_t *)context;

    publisher->count++;
    publisher->last_timestamp_ms = timestamp_ms;
    return !publisher->reject;
}

static void seed_sample(fake_lsm6ds3_bus_t *bus)
{
    const uint8_t sample[] = {
        0x34, 0x12, 0xFE, 0xFF, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0xFF, 0x7F,
    };

    memcpy(&bus->registers[0x22], sample, sizeof(sample));
}

static void test_init_configures_device(void)
{
    fake_lsm6ds3_bus_t bus = { 0 };
    watch_lsm6ds3_bus_t bus_api;
    watch_lsm6ds3_t device;

    bus.registers[0x0F] = WATCH_LSM6DS3_WHO_AM_I_VALUE;
    bus_api = fake_bus(&bus);
    assert(watch_lsm6ds3_init(&device, &bus_api) == WATCH_LSM6DS3_RESULT_OK);
    assert(device.initialized);
    assert(device.who_am_i == WATCH_LSM6DS3_WHO_AM_I_VALUE);
    assert(bus.write_count == 3U);
    assert(bus.write_registers[0] == 0x10U);
    assert(bus.write_values[0] == 0x48U);
    assert(bus.write_registers[1] == 0x11U);
    assert(bus.write_values[1] == 0x44U);
    assert(bus.write_registers[2] == 0x12U);
    assert(bus.write_values[2] == 0x44U);
}

static void test_wrong_id_is_rejected(void)
{
    fake_lsm6ds3_bus_t bus = { 0 };
    watch_lsm6ds3_bus_t bus_api;
    watch_lsm6ds3_t device;

    bus.registers[0x0F] = 0x00U;
    bus_api = fake_bus(&bus);
    assert(watch_lsm6ds3_init(&device, &bus_api) == WATCH_LSM6DS3_RESULT_ID_MISMATCH);
    assert(!device.initialized);
}

static void test_sample_is_decoded_little_endian(void)
{
    fake_lsm6ds3_bus_t bus = { 0 };
    watch_lsm6ds3_bus_t bus_api;
    watch_lsm6ds3_t device;
    watch_lsm6ds3_sample_t sample;

    bus.registers[0x0F] = WATCH_LSM6DS3_WHO_AM_I_VALUE;
    seed_sample(&bus);
    bus_api = fake_bus(&bus);
    assert(watch_lsm6ds3_init(&device, &bus_api) == WATCH_LSM6DS3_RESULT_OK);
    assert(watch_lsm6ds3_read_sample(&device, &sample) == WATCH_LSM6DS3_RESULT_OK);
    assert(sample.gyro_x == 0x1234);
    assert(sample.gyro_y == -2);
    assert(sample.gyro_z == INT16_MIN);
    assert(sample.accel_x == 1);
    assert(sample.accel_y == INT16_MIN);
    assert(sample.accel_z == INT16_MAX);
}

static void test_service_is_periodic_and_reports_drops(void)
{
    fake_lsm6ds3_bus_t bus = { 0 };
    fake_publisher_t publisher = { 0 };
    watch_lsm6ds3_bus_t bus_api;
    watch_lsm6ds3_service_t service;
    watch_lsm6ds3_service_status_t status;

    bus.registers[0x0F] = WATCH_LSM6DS3_WHO_AM_I_VALUE;
    seed_sample(&bus);
    bus_api = fake_bus(&bus);
    assert(watch_lsm6ds3_service_init(&service, &bus_api, fake_publish, &publisher));
    assert(watch_lsm6ds3_service_process(&service, 0U));
    assert(!watch_lsm6ds3_service_process(&service, 10U));
    assert(watch_lsm6ds3_service_process(&service, 20U));
    assert(publisher.count == 2U);
    assert(publisher.last_timestamp_ms == 20U);

    publisher.reject = true;
    assert(watch_lsm6ds3_service_process(&service, 40U));
    assert(watch_lsm6ds3_service_read_status(&service, &status));
    assert(status.ready);
    assert(status.sample_valid);
    assert(status.sample_count == 3U);
    assert(status.event_drop_count == 1U);
}

int main(void)
{
    test_init_configures_device();
    test_wrong_id_is_rejected();
    test_sample_is_decoded_little_endian();
    test_service_is_periodic_and_reports_drops();
    return 0;
}
