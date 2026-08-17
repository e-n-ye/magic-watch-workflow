#include "watch_aht20.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    uint8_t response[WATCH_AHT20_MEASUREMENT_RESPONSE_LENGTH];
    uint8_t writes[8][3];
    uint8_t write_lengths[8];
    uint8_t write_count;
    bool fail_read;
    bool busy;
} fake_aht20_bus_t;

typedef struct
{
    uint32_t count;
    uint32_t timestamp_ms;
    bool reject;
} fake_publisher_t;

static uint8_t test_crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFFU;

    for (uint8_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U ? (uint8_t)((crc << 1U) ^ 0x31U)
                                      : (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

static bool fake_write(void *context, uint8_t address, const uint8_t *data, uint8_t length)
{
    fake_aht20_bus_t *bus = (fake_aht20_bus_t *)context;

    assert(address == WATCH_AHT20_I2C_ADDRESS);
    assert(length <= sizeof(bus->writes[0]));
    if (bus->write_count < sizeof(bus->writes) / sizeof(bus->writes[0])) {
        memcpy(bus->writes[bus->write_count], data, length);
        bus->write_lengths[bus->write_count] = length;
        bus->write_count++;
    }
    return true;
}

static bool fake_read(void *context, uint8_t address, uint8_t *data, uint8_t length)
{
    fake_aht20_bus_t *bus = (fake_aht20_bus_t *)context;

    assert(address == WATCH_AHT20_I2C_ADDRESS);
    if (bus->fail_read) {
        return false;
    }
    if (length == 1U) {
        data[0] = bus->busy ? WATCH_AHT20_STATUS_BUSY : bus->response[0];
    } else {
        memcpy(data, bus->response, length);
        if (bus->busy) {
            data[0] = WATCH_AHT20_STATUS_BUSY;
        }
    }
    return true;
}

static bool fake_publish(void *context, uint32_t timestamp_ms)
{
    fake_publisher_t *publisher = (fake_publisher_t *)context;

    publisher->count++;
    publisher->timestamp_ms = timestamp_ms;
    return !publisher->reject;
}

static void make_response(fake_aht20_bus_t *bus)
{
    bus->response[0] = 0x18U;
    bus->response[1] = 0x80U;
    bus->response[2] = 0x00U;
    bus->response[3] = 0x06U;
    bus->response[4] = 0x00U;
    bus->response[5] = 0x00U;
    bus->response[6] = test_crc8(bus->response, 6U);
}

static watch_aht20_bus_t fake_bus(fake_aht20_bus_t *bus)
{
    return (watch_aht20_bus_t) {
        .write = fake_write,
        .read = fake_read,
        .context = bus,
    };
}

static void test_crc_and_decode(void)
{
    fake_aht20_bus_t bus = { 0 };
    watch_aht20_sample_t sample;

    make_response(&bus);
    assert(watch_aht20_validate_crc(bus.response));
    assert(watch_aht20_decode_measurement(bus.response, &sample));
    assert(sample.humidity_centi_percent == 5000U);
    assert(sample.temperature_centi_c == 2500);

    bus.response[6] ^= 0x01U;
    assert(!watch_aht20_validate_crc(bus.response));
    assert(!watch_aht20_decode_measurement(bus.response, &sample));
}

static void test_service_initializes_and_publishes(void)
{
    fake_aht20_bus_t bus = { 0 };
    fake_publisher_t publisher = { 0 };
    watch_aht20_service_t service;
    watch_aht20_service_status_t status;
    watch_aht20_sample_t sample;
    watch_aht20_bus_t bus_api = fake_bus(&bus);

    make_response(&bus);
    assert(watch_aht20_service_init(&service, &bus_api, fake_publish, &publisher));
    assert(watch_aht20_service_process(&service, 0U));
    assert(bus.write_count == 1U);
    assert(bus.write_lengths[0] == 3U);
    assert(bus.writes[0][0] == WATCH_AHT20_COMMAND_INITIALIZE);
    assert(watch_aht20_service_process(&service, 10U));
    assert(bus.write_count == 2U);
    assert(bus.writes[1][0] == WATCH_AHT20_COMMAND_TRIGGER);
    assert(watch_aht20_service_process(&service, 20U));
    assert(watch_aht20_service_read_status(&service, &status));
    assert(status.ready);
    assert(status.calibrated);
    assert(status.sample_valid);
    assert(status.sample_count == 1U);
    assert(status.read_error_count == 0U);
    assert(status.crc_error_count == 0U);
    assert(watch_aht20_service_read_latest(&service, &sample));
    assert(sample.humidity_centi_percent == 5000U);
    assert(sample.temperature_centi_c == 2500);
    assert(publisher.count == 1U);
    assert(publisher.timestamp_ms == 20U);

    publisher.reject = true;
    assert(watch_aht20_service_process(&service, 1020U));
    assert(watch_aht20_service_process(&service, 1030U));
    assert(watch_aht20_service_read_status(&service, &status));
    assert(status.sample_count == 2U);
    assert(status.event_drop_count == 1U);
}

static void test_service_reports_busy_timeout_and_read_error(void)
{
    fake_aht20_bus_t bus = { 0 };
    watch_aht20_service_t service;
    watch_aht20_service_status_t status;
    watch_aht20_bus_t bus_api;

    make_response(&bus);
    bus.busy = true;
    bus_api = fake_bus(&bus);
    assert(watch_aht20_service_init(&service, &bus_api, NULL, NULL));
    assert(watch_aht20_service_process(&service, 0U));
    assert(!watch_aht20_service_process(&service, 100U));
    assert(watch_aht20_service_read_status(&service, &status));
    assert(status.state == WATCH_AHT20_SERVICE_STATE_FAILED);
    assert(status.timeout_count == 1U);

    bus.busy = false;
    bus.fail_read = true;
    assert(watch_aht20_service_init(&service, &bus_api, NULL, NULL));
    assert(watch_aht20_service_process(&service, 200U));
    assert(!watch_aht20_service_process(&service, 210U));
    assert(watch_aht20_service_read_status(&service, &status));
    assert(status.state == WATCH_AHT20_SERVICE_STATE_FAILED);
    assert(status.read_error_count == 1U);
}

int main(void)
{
    test_crc_and_decode();
    test_service_initializes_and_publishes();
    test_service_reports_busy_timeout_and_read_error();
    return 0;
}
