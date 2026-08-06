#include "watch_power.h"

#include "main.h"

void watch_power_latch_early(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* Keep the watch power latch asserted before CubeMX initializes GPIO. */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio_init.Pin = POWER_EN_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(POWER_EN_GPIO_Port, POWER_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_Init(POWER_EN_GPIO_Port, &gpio_init);
}
