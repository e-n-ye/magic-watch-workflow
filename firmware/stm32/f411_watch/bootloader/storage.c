#include "storage.h"

#include <string.h>

#include "stm32f411xe.h"

#include "watch_ota_metadata.h"
#include "watch_w25_partitions.h"

#define BOOTLOADER_SPI_TIMEOUT_LOOPS 1000000UL
#define BOOTLOADER_W25_TIMEOUT_MS 5000U
#define BOOTLOADER_FLASH_KEY1 0x45670123UL
#define BOOTLOADER_FLASH_KEY2 0xCDEF89ABUL

#define F411_FLASH_SECTOR_4 4U
#define F411_FLASH_SECTOR_5 5U
#define F411_FLASH_SECTOR_6 6U
#define F411_FLASH_SECTOR_7 7U

static watch_w25q128_t s_w25;
static watch_ota_metadata_t s_metadata;
static uint32_t s_time_ms;
static bool s_initialized;

static void bootloader_watchdog_refresh(void)
{
    IWDG->KR = 0xAAAAU;
}

void f411_bootloader_watchdog_extend(void)
{
    IWDG->KR = 0x5555U;
    IWDG->PR = IWDG_PR_PR_0 | IWDG_PR_PR_1 | IWDG_PR_PR_2;
    IWDG->RLR = 0x0FFFU;
    bootloader_watchdog_refresh();
}

void watch_ota_package_progress(void)
{
    bootloader_watchdog_refresh();
}

void watch_ota_install_progress(void)
{
    bootloader_watchdog_refresh();
}

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- > 0U) {
        __NOP();
    }
}

static uint32_t w25_now_ms(void *context)
{
    (void)context;
    return s_time_ms;
}

static void w25_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    while (delay_ms-- > 0U) {
        bootloader_watchdog_refresh();
        delay_cycles(16000U);
        ++s_time_ms;
    }
}

static void w25_cs(bool selected)
{
    if (selected) {
        GPIOA->BSRR = GPIO_BSRR_BR15;
    } else {
        GPIOA->BSRR = GPIO_BSRR_BS15;
    }
}

static bool spi_transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
    uint32_t timeout;

    (void)context;

    if (tx == NULL || rx == NULL || length == 0U) {
        return false;
    }

    w25_cs(true);
    for (size_t index = 0U; index < length; ++index) {
        bootloader_watchdog_refresh();
        timeout = BOOTLOADER_SPI_TIMEOUT_LOOPS;
        while ((SPI3->SR & SPI_SR_TXE) == 0U && timeout-- > 0U) {
            bootloader_watchdog_refresh();
        }
        if (timeout == 0U) {
            w25_cs(false);
            return false;
        }
        *(__IO uint8_t *)&SPI3->DR = tx[index];

        timeout = BOOTLOADER_SPI_TIMEOUT_LOOPS;
        while ((SPI3->SR & SPI_SR_RXNE) == 0U && timeout-- > 0U) {
            bootloader_watchdog_refresh();
        }
        if (timeout == 0U) {
            w25_cs(false);
            return false;
        }
        rx[index] = *(__IO uint8_t *)&SPI3->DR;
    }

    timeout = BOOTLOADER_SPI_TIMEOUT_LOOPS;
    while ((SPI3->SR & SPI_SR_BSY) != 0U && timeout-- > 0U) {
        bootloader_watchdog_refresh();
    }
    bootloader_watchdog_refresh();
    w25_cs(false);
    return timeout != 0U;
}

static void configure_gpio_spi(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (15U * 2U))) | (1UL << (15U * 2U));
    GPIOA->OTYPER &= ~(1UL << 15U);
    GPIOA->OSPEEDR |= 3UL << (15U * 2U);
    GPIOA->PUPDR &= ~(3UL << (15U * 2U));
    GPIOA->BSRR = GPIO_BSRR_BS15;

    for (uint32_t pin = 3U; pin <= 5U; ++pin) {
        GPIOB->MODER = (GPIOB->MODER & ~(3UL << (pin * 2U))) | (2UL << (pin * 2U));
        GPIOB->OSPEEDR |= 3UL << (pin * 2U);
        GPIOB->PUPDR &= ~(3UL << (pin * 2U));
        GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFUL << (pin * 4U))) | (6UL << (pin * 4U));
    }

    SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1;
    SPI3->CR2 = 0U;
    SPI3->CR1 |= SPI_CR1_SPE;
}

static bool flash_wait_ready(void)
{
    uint32_t timeout = BOOTLOADER_SPI_TIMEOUT_LOOPS;

    while ((FLASH->SR & FLASH_SR_BSY) != 0U && timeout-- > 0U) {
        bootloader_watchdog_refresh();
    }
    bootloader_watchdog_refresh();
    return timeout != 0U;
}

static void flash_unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0U) {
        FLASH->KEYR = BOOTLOADER_FLASH_KEY1;
        FLASH->KEYR = BOOTLOADER_FLASH_KEY2;
    }
}

static bool flash_clear_errors(void)
{
    const uint32_t flags = FLASH_SR_EOP | FLASH_SR_SOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR
        | FLASH_SR_PGPERR | FLASH_SR_PGSERR;

    FLASH->SR = flags;
    return (FLASH->SR
            & (FLASH_SR_SOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR
               | FLASH_SR_PGSERR))
        == 0U;
}

static unsigned int app_sector_for_offset(uint32_t offset)
{
    uint32_t address = F411_BOOTLOADER_APP_BASE + offset;

    if (address < 0x08020000UL) {
        return F411_FLASH_SECTOR_4;
    }
    if (address < 0x08040000UL) {
        return F411_FLASH_SECTOR_5;
    }
    if (address < 0x08060000UL) {
        return F411_FLASH_SECTOR_6;
    }
    return F411_FLASH_SECTOR_7;
}

static bool app_sector_boundary(uint32_t offset)
{
    return offset == 0U || offset == 0x10000UL || offset == 0x30000UL || offset == 0x50000UL;
}

static bool flash_erase_app_sector(uint32_t offset)
{
    if (!app_sector_boundary(offset) || !flash_wait_ready()) {
        return false;
    }
    flash_unlock();
    if (!flash_clear_errors()) {
        return false;
    }
    FLASH->CR = (FLASH->CR & ~(FLASH_CR_SNB | FLASH_CR_PSIZE)) | FLASH_CR_SER | FLASH_CR_PSIZE_1
        | ((uint32_t)app_sector_for_offset(offset) << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;
    if (!flash_wait_ready()) {
        FLASH->CR &= ~FLASH_CR_SER;
        return false;
    }
    FLASH->CR &= ~FLASH_CR_SER;
    return flash_clear_errors();
}

static uint32_t app_sector_start(uint32_t offset)
{
    if (offset < 0x10000UL) {
        return 0U;
    }
    if (offset < 0x30000UL) {
        return 0x10000UL;
    }
    if (offset < 0x50000UL) {
        return 0x30000UL;
    }
    return 0x50000UL;
}

static bool flash_program(uint32_t offset, const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0U || (offset & 3U) != 0U || (length & 3U) != 0U
        || offset > F411_BOOTLOADER_APP_SIZE || length > F411_BOOTLOADER_APP_SIZE - offset) {
        return false;
    }
    if (!flash_wait_ready()) {
        return false;
    }
    flash_unlock();
    if (!flash_clear_errors()) {
        return false;
    }
    FLASH->CR = (FLASH->CR & ~FLASH_CR_PSIZE) | FLASH_CR_PSIZE_1 | FLASH_CR_PG;
    for (size_t index = 0U; index < length; index += sizeof(uint32_t)) {
        bootloader_watchdog_refresh();
        uint32_t word;
        memcpy(&word, data + index, sizeof(word));
        *(__IO uint32_t *)(F411_BOOTLOADER_APP_BASE + offset + index) = word;
        if (!flash_wait_ready()
            || *(__IO const uint32_t *)(F411_BOOTLOADER_APP_BASE + offset + index) != word) {
            FLASH->CR &= ~FLASH_CR_PG;
            return false;
        }
    }
    FLASH->CR &= ~FLASH_CR_PG;
    return flash_clear_errors();
}

bool f411_bootloader_storage_init(void)
{
    const watch_w25q128_bus_t bus = {
        .transfer = spi_transfer,
        .now_ms = w25_now_ms,
        .delay_ms = w25_delay_ms,
        .context = NULL,
    };

    if (s_initialized) {
        return true;
    }
    configure_gpio_spi();
    if (!watch_w25q128_init(&s_w25, &bus) || !watch_ota_metadata_init(&s_metadata, &s_w25)) {
        return false;
    }
    s_initialized = true;
    return true;
}

watch_w25q128_t *f411_bootloader_w25(void)
{
    return s_initialized ? &s_w25 : NULL;
}

watch_ota_metadata_t *f411_bootloader_metadata(void)
{
    return s_initialized ? &s_metadata : NULL;
}

bool f411_bootloader_storage_read(void *context, watch_ota_install_region_t region, uint32_t offset,
                                  uint8_t *data, size_t length)
{
    (void)context;
    if (data == NULL || length == 0U || offset > F411_BOOTLOADER_APP_SIZE
        || length > F411_BOOTLOADER_APP_SIZE - offset) {
        return false;
    }
    switch (region) {
    case WATCH_OTA_INSTALL_REGION_APP:
        memcpy(data, (const void *)(F411_BOOTLOADER_APP_BASE + offset), length);
        return true;
    case WATCH_OTA_INSTALL_REGION_ROLLBACK:
        return watch_w25q128_read(&s_w25, F411_BOOTLOADER_ROLLBACK_BASE + offset, data, length,
                                  BOOTLOADER_W25_TIMEOUT_MS)
            == WATCH_W25Q128_RESULT_OK;
    case WATCH_OTA_INSTALL_REGION_CANDIDATE:
        return watch_w25q128_read(&s_w25, F411_BOOTLOADER_CANDIDATE_BASE + offset, data, length,
                                  BOOTLOADER_W25_TIMEOUT_MS)
            == WATCH_W25Q128_RESULT_OK;
    case WATCH_OTA_INSTALL_REGION_COUNT:
        break;
    }
    return false;
}

bool f411_bootloader_storage_erase(void *context, watch_ota_install_region_t region,
                                   uint32_t offset, size_t length)
{
    (void)context;
    if (length != WATCH_OTA_INSTALL_SECTOR_SIZE || offset % WATCH_OTA_INSTALL_SECTOR_SIZE != 0U
        || offset > F411_BOOTLOADER_APP_SIZE - WATCH_OTA_INSTALL_SECTOR_SIZE) {
        return false;
    }
    switch (region) {
    case WATCH_OTA_INSTALL_REGION_APP:
        if (!app_sector_boundary(offset)) {
            return true;
        }
        return flash_erase_app_sector(offset);
    case WATCH_OTA_INSTALL_REGION_ROLLBACK:
        for (uint32_t address = F411_BOOTLOADER_ROLLBACK_BASE;
             address < F411_BOOTLOADER_ROLLBACK_BASE + F411_BOOTLOADER_APP_SIZE;
             address += WATCH_W25Q128_SECTOR_SIZE) {
            if (watch_w25q128_sector_erase(&s_w25, address, BOOTLOADER_W25_TIMEOUT_MS)
                != WATCH_W25Q128_RESULT_OK) {
                return false;
            }
        }
        return true;
    case WATCH_OTA_INSTALL_REGION_CANDIDATE:
        for (uint32_t address = F411_BOOTLOADER_CANDIDATE_BASE;
             address < F411_BOOTLOADER_CANDIDATE_BASE + F411_BOOTLOADER_APP_SIZE;
             address += WATCH_W25Q128_SECTOR_SIZE) {
            if (watch_w25q128_sector_erase(&s_w25, address, BOOTLOADER_W25_TIMEOUT_MS)
                != WATCH_W25Q128_RESULT_OK) {
                return false;
            }
        }
        return true;
    case WATCH_OTA_INSTALL_REGION_COUNT:
        break;
    }
    return false;
}

bool f411_bootloader_storage_write(void *context, watch_ota_install_region_t region,
                                   uint32_t offset, const uint8_t *data, size_t length)
{
    (void)context;
    if (data == NULL || length == 0U || offset > F411_BOOTLOADER_APP_SIZE
        || length > F411_BOOTLOADER_APP_SIZE - offset) {
        return false;
    }
    switch (region) {
    case WATCH_OTA_INSTALL_REGION_APP:
        return flash_program(offset, data, length);
    case WATCH_OTA_INSTALL_REGION_ROLLBACK:
        for (size_t written = 0U; written < length;) {
            uint32_t address = F411_BOOTLOADER_ROLLBACK_BASE + offset + (uint32_t)written;
            size_t page = WATCH_W25Q128_PAGE_SIZE - (address % WATCH_W25Q128_PAGE_SIZE);
            size_t chunk = length - written < page ? length - written : page;
            if (watch_w25q128_page_program(&s_w25, address, data + written, chunk,
                                           BOOTLOADER_W25_TIMEOUT_MS)
                != WATCH_W25Q128_RESULT_OK) {
                return false;
            }
            written += chunk;
        }
        return true;
    case WATCH_OTA_INSTALL_REGION_CANDIDATE:
        return false;
    case WATCH_OTA_INSTALL_REGION_COUNT:
        break;
    }
    return false;
}

bool f411_bootloader_storage_prepare_resume(watch_ota_install_region_t region, uint32_t *progress)
{
    uint32_t boundary;

    if (progress == NULL || *progress > F411_BOOTLOADER_APP_SIZE
        || (region != WATCH_OTA_INSTALL_REGION_APP
            && region != WATCH_OTA_INSTALL_REGION_ROLLBACK)) {
        return false;
    }
    if (*progress == 0U) {
        return true;
    }
    boundary = region == WATCH_OTA_INSTALL_REGION_APP
        ? app_sector_start(*progress)
        : *progress & ~(WATCH_W25Q128_SECTOR_SIZE - 1U);
    *progress = boundary;
    return true;
}

bool f411_bootloader_storage_persist(void *context, const watch_ota_metadata_record_t *record)
{
    (void)context;
    return watch_ota_metadata_commit(&s_metadata, record) == WATCH_OTA_METADATA_RESULT_OK;
}

bool f411_bootloader_candidate_read(void *context, uint32_t offset, uint8_t *data, size_t length)
{
    (void)context;
    if (data == NULL || length == 0U || offset > WATCH_OTA_PACKAGE_SIZE
        || length > WATCH_OTA_PACKAGE_SIZE - offset) {
        return false;
    }
    return watch_w25q128_read(&s_w25, F411_BOOTLOADER_CANDIDATE_BASE + offset, data, length,
                              BOOTLOADER_W25_TIMEOUT_MS)
        == WATCH_W25Q128_RESULT_OK;
}
