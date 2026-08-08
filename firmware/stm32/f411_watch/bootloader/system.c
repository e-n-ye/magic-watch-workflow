#include "stm32f411xe.h"

#define BOOTLOADER_FLASH_START 0x08000000UL

void SystemInit(void)
{
#if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
    SCB->CPACR |= (3UL << (10U * 2U)) | (3UL << (11U * 2U));
#endif

    SCB->VTOR = BOOTLOADER_FLASH_START;
    __DSB();
    __ISB();
}
