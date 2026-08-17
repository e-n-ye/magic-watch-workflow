#include "watch_lis2mdl.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define TEST_REG_FUNC_CFG_ACCESS 0x01U
#define TEST_REG_MASTER_CONFIG 0x1AU
#define TEST_REG_SENSORHUB1 0x2EU
#define TEST_REG_FUNC_SRC 0x53U
#define TEST_REG_FUNC_SRC2 0x54U
#define TEST_EMBEDDED_BANK_A 0x80U
#define TEST_REG_SLV0_ADD 0x02U
#define TEST_REG_SLV0_SUBADD 0x03U
#define TEST_REG_SLAVE0_CONFIG 0x04U
#define TEST_REG_DATAWRITE_SLV0 0x0EU

typedef struct
{
    uint8_t user[256];
    uint8_t embedded[256];
    uint8_t writes_reg[64];
    uint8_t writes_value[64];
    uint8_t writes_count;
    bool bank_a;
    bool read_failed;
    bool write_failed;
} fake_hub_bus_t;

typedef struct
{
    uint32_t count;
    uint32_t timestamp_ms;
    bool reject;
} fake_publisher_t;

static bool fake_read(void *context, uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    fake_hub_bus_t *bus = (fake_hub_bus_t *)context;

    assert(address == WATCH_LSM6DS3_I2C_ADDRESS);
    if (bus->read_failed) {
        return false;
    }
    memcpy(data, bus->bank_a ? &bus->embedded[reg] : &bus->user[reg], length);
    return true;
}

static bool fake_write(void *context, uint8_t address, uint8_t reg, const uint8_t *data,
                       uint8_t length)
{
    fake_hub_bus_t *bus = (fake_hub_bus_t *)context;

    assert(address == WATCH_LSM6DS3_I2C_ADDRESS);
    assert(length == 1U);
    if (bus->write_failed) {
        return false;
    }
    if (bus->writes_count < sizeof(bus->writes_reg)) {
        bus->writes_reg[bus->writes_count] = reg;
        bus->writes_value[bus->writes_count] = data[0];
        bus->writes_count++;
    }
    if (reg == TEST_REG_FUNC_CFG_ACCESS) {
        bus->bank_a = data[0] == TEST_EMBEDDED_BANK_A;
    }
    if (bus->bank_a) {
        bus->embedded[reg] = data[0];
    } else {
        bus->user[reg] = data[0];
    }
    return true;
}

static watch_lsm6ds3_bus_t fake_bus(fake_hub_bus_t *bus)
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
    publisher->timestamp_ms = timestamp_ms;
    return !publisher->reject;
}

static void seed_hub_cache(fake_hub_bus_t *bus, const uint8_t *data, uint8_t length)
{
    memcpy(&bus->user[TEST_REG_SENSORHUB1], data, length);
}

static void test_identity_and_sample_decode(void)
{
    const uint8_t data[] = { 0x34U, 0x12U, 0xFEU, 0xFFU, 0x00U, 0x80U };
    watch_lis2mdl_sample_t sample;

    assert(watch_lis2mdl_validate_identity(WATCH_LIS2MDL_WHO_AM_I_VALUE));
    assert(!watch_lis2mdl_validate_identity(0x00U));
    assert(watch_lis2mdl_decode_sample(data, &sample));
    assert(sample.magnetic_x == 0x1234);
    assert(sample.magnetic_y == -2);
    assert(sample.magnetic_z == INT16_MIN);
}

static void test_hub_read_config_uses_bank_a_and_waits(void)
{
    fake_hub_bus_t bus = { 0 };
    watch_lsm6ds3_sensor_hub_t hub;
    watch_lsm6ds3_bus_t bus_api = fake_bus(&bus);

    assert(watch_lsm6ds3_sensor_hub_init(&hub, &bus_api));
    assert(watch_lsm6ds3_sensor_hub_configure_read(&hub, WATCH_LIS2MDL_I2C_ADDRESS, 0x4FU, 1U, 100U)
           == WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK);
    assert(bus.embedded[TEST_REG_SLV0_ADD] == 0x3DU);
    assert(bus.embedded[TEST_REG_SLV0_SUBADD] == 0x4FU);
    assert(bus.embedded[TEST_REG_SLAVE0_CONFIG] == 0x01U);
    assert(bus.user[TEST_REG_MASTER_CONFIG] == 0x09U);
    assert(!bus.bank_a);
    assert(watch_lsm6ds3_sensor_hub_wait(&hub, 119U) == WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY);
    assert(watch_lsm6ds3_sensor_hub_wait(&hub, 120U) == WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY);
    bus.user[TEST_REG_FUNC_SRC] = 0x01U;
    assert(watch_lsm6ds3_sensor_hub_wait(&hub, 120U) == WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK);
}

static void test_hub_write_and_nack_are_reported(void)
{
    fake_hub_bus_t bus = { 0 };
    watch_lsm6ds3_sensor_hub_t hub;
    watch_lsm6ds3_bus_t bus_api = fake_bus(&bus);

    assert(watch_lsm6ds3_sensor_hub_init(&hub, &bus_api));
    assert(watch_lsm6ds3_sensor_hub_configure_write(&hub, WATCH_LIS2MDL_I2C_ADDRESS, 0x60U, 0x80U, 0U)
           == WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK);
    assert(bus.embedded[TEST_REG_SLV0_ADD] == 0x3CU);
    assert(bus.embedded[TEST_REG_SLV0_SUBADD] == 0x60U);
    assert(bus.embedded[TEST_REG_DATAWRITE_SLV0] == 0x80U);
    bus.user[TEST_REG_FUNC_SRC] = 0x01U;
    bus.user[TEST_REG_FUNC_SRC2] = 0x08U;
    assert(watch_lsm6ds3_sensor_hub_wait(&hub, 20U) == WATCH_LSM6DS3_SENSOR_HUB_RESULT_NACK);
}

static void test_hub_times_out_when_operation_never_completes(void)
{
    fake_hub_bus_t bus = { 0 };
    watch_lsm6ds3_sensor_hub_t hub;
    watch_lsm6ds3_bus_t bus_api = fake_bus(&bus);

    assert(watch_lsm6ds3_sensor_hub_init(&hub, &bus_api));
    assert(watch_lsm6ds3_sensor_hub_configure_read(&hub, WATCH_LIS2MDL_I2C_ADDRESS, 0x4FU, 1U, 0U)
           == WATCH_LSM6DS3_SENSOR_HUB_RESULT_OK);
    assert(watch_lsm6ds3_sensor_hub_wait(&hub, 99U) == WATCH_LSM6DS3_SENSOR_HUB_RESULT_NOT_READY);
    assert(watch_lsm6ds3_sensor_hub_wait(&hub, 100U) == WATCH_LSM6DS3_SENSOR_HUB_RESULT_BUS_ERROR);
}

static void test_service_configures_then_publishes_periodically(void)
{
    const uint8_t id[] = { WATCH_LIS2MDL_WHO_AM_I_VALUE };
    const uint8_t sample[] = { 0x01U, 0x00U, 0xFEU, 0xFFU, 0xFFU, 0x7FU };
    fake_hub_bus_t bus = { 0 };
    fake_publisher_t publisher = { 0 };
    watch_lis2mdl_service_t service;
    watch_lis2mdl_service_status_t status;
    watch_lis2mdl_sample_t latest;
    watch_lsm6ds3_bus_t bus_api = fake_bus(&bus);

    assert(watch_lis2mdl_service_init(&service, &bus_api, fake_publish, &publisher));
    assert(watch_lis2mdl_service_process(&service, 0U));
    seed_hub_cache(&bus, id, sizeof(id));
    assert(!watch_lis2mdl_service_process(&service, 19U));
    bus.user[TEST_REG_FUNC_SRC] = 0x01U;
    assert(watch_lis2mdl_service_process(&service, 20U));
    assert(watch_lis2mdl_service_process(&service, 40U));
    assert(watch_lis2mdl_service_process(&service, 60U));
    assert(watch_lis2mdl_service_process(&service, 80U));
    seed_hub_cache(&bus, sample, sizeof(sample));
    assert(watch_lis2mdl_service_process(&service, 100U));
    assert(watch_lis2mdl_service_read_status(&service, &status));
    assert(status.ready);
    assert(status.who_am_i == WATCH_LIS2MDL_WHO_AM_I_VALUE);
    assert(status.sample_count == 1U);
    assert(watch_lis2mdl_service_read_latest(&service, &latest));
    assert(latest.magnetic_x == 1);
    assert(latest.magnetic_y == -2);
    assert(latest.magnetic_z == INT16_MAX);
    assert(publisher.count == 1U);
    assert(!watch_lis2mdl_service_process(&service, 199U));

    publisher.reject = true;
    assert(!watch_lis2mdl_service_process(&service, 200U));
    assert(watch_lis2mdl_service_process(&service, 220U));
    assert(watch_lis2mdl_service_read_status(&service, &status));
    assert(status.sample_count == 2U);
    assert(status.event_drop_count == 1U);
}

static void test_service_rejects_wrong_id_and_nack(void)
{
    const uint8_t wrong_id[] = { 0x00U };
    fake_hub_bus_t bus = { 0 };
    watch_lis2mdl_service_t service;
    watch_lis2mdl_service_status_t status;
    watch_lsm6ds3_bus_t bus_api = fake_bus(&bus);

    assert(watch_lis2mdl_service_init(&service, &bus_api, NULL, NULL));
    assert(watch_lis2mdl_service_process(&service, 0U));
    seed_hub_cache(&bus, wrong_id, sizeof(wrong_id));
    bus.user[TEST_REG_FUNC_SRC] = 0x01U;
    assert(!watch_lis2mdl_service_process(&service, 20U));
    assert(watch_lis2mdl_service_read_status(&service, &status));
    assert(!status.ready);
    assert(status.state == WATCH_LIS2MDL_SERVICE_STATE_FAILED);
    assert(status.read_error_count == 1U);

    assert(watch_lis2mdl_service_init(&service, &bus_api, NULL, NULL));
    assert(watch_lis2mdl_service_process(&service, 100U));
    bus.user[TEST_REG_FUNC_SRC] = 0x01U;
    bus.user[TEST_REG_FUNC_SRC2] = 0x08U;
    assert(!watch_lis2mdl_service_process(&service, 120U));
    assert(watch_lis2mdl_service_read_status(&service, &status));
    assert(status.nack_count == 1U);
}

int main(void)
{
    test_identity_and_sample_decode();
    test_hub_read_config_uses_bank_a_and_waits();
    test_hub_write_and_nack_are_reported();
    test_hub_times_out_when_operation_never_completes();
    test_service_configures_then_publishes_periodically();
    test_service_rejects_wrong_id_and_nack();
    return 0;
}
