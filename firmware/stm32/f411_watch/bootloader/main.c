#include <stdint.h>

#include "stm32f411xe.h"
#include "manifest.h"
#include "ota.h"

typedef void (*bootloader_reset_handler_t)(void);

#define APPLICATION_FLASH_START 0x08010000UL

__attribute__((noreturn)) static void bootloader_halt(void)
{
    __disable_irq();

    while (1) {
        __NOP();
    }
}

__attribute__((noreturn)) static void bootloader_jump_to_application(void)
{
    volatile const uint32_t *vector_table = (volatile const uint32_t *)APPLICATION_FLASH_START;
    uint32_t main_stack_pointer = vector_table[0];
    uint32_t reset_vector = vector_table[1];
    bootloader_reset_handler_t reset_handler = (bootloader_reset_handler_t)(uintptr_t)reset_vector;

    __disable_irq();
    SysTick->CTRL = 0UL;
    SysTick->LOAD = 0UL;
    SysTick->VAL = 0UL;

    for (uint32_t index = 0U; index < 8U; index++) {
        NVIC->ICER[index] = 0xFFFFFFFFUL;
        NVIC->ICPR[index] = 0xFFFFFFFFUL;
    }

    SCB->VTOR = APPLICATION_FLASH_START;
    __DSB();
    __ISB();
    __set_MSP(main_stack_pointer);
    /* The application initializes and relies on interrupt-driven HAL services. */
    __enable_irq();
    reset_handler();

    while (1) {
        __NOP();
    }
}

int main(void)
{
    int ota_result = f411_bootloader_process_ota();

    if (ota_result < 0) {
        bootloader_halt();
    }
    if (bootloader_application_is_valid()) {
        bootloader_jump_to_application();
    }

    bootloader_halt();
}
