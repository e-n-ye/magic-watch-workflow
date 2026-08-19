#include "stm32f411xe.h"

#define BOOTLOADER_FLASH_START 0x08000000UL

void SystemInit(void)
{
    /* The application starts IWDG before handing control to the bootloader on
     * reset. Extend
     * it before any W25/Flash transaction can exceed the old
     * application timeout. */
    IWDG->KR = 0x5555U;
    IWDG->PR = IWDG_PR_PR_0 | IWDG_PR_PR_1 | IWDG_PR_PR_2;
    IWDG->RLR = 0x0FFFU;
    IWDG->KR = 0xAAAAU;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
#if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
    SCB->CPACR |= (3UL << (10U * 2U)) | (3UL << (11U * 2U));
#endif

    SCB->VTOR = BOOTLOADER_FLASH_START;
    __DSB();
    __ISB();
}
