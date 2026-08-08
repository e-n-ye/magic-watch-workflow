#include <stdint.h>

#include "stm32f411xe.h"

#define APPLICATION_FLASH_START 0x08010000UL
#define APPLICATION_EXECUTION_END 0x0807F000UL
#define SRAM_START 0x20000000UL
#define SRAM_END 0x20020000UL

typedef void (*bootloader_reset_handler_t)(void);

static int bootloader_application_is_valid(void)
{
    volatile const uint32_t *vector_table = (volatile const uint32_t *)APPLICATION_FLASH_START;
    uint32_t main_stack_pointer = vector_table[0];
    uint32_t reset_vector = vector_table[1];
    uint32_t reset_address = reset_vector & ~1UL;

    if ((main_stack_pointer & 0x7UL) != 0UL || main_stack_pointer < SRAM_START
        || main_stack_pointer > SRAM_END) {
        return 0;
    }

    if ((reset_vector & 1UL) == 0UL || reset_address < APPLICATION_FLASH_START
        || reset_address >= APPLICATION_EXECUTION_END) {
        return 0;
    }

    return 1;
}

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
    if (bootloader_application_is_valid()) {
        bootloader_jump_to_application();
    }

    bootloader_halt();
}
