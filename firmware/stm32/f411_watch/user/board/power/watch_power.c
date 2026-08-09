#include "watch_power.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "rtc.h"
#include "task.h"
#include "board/display/watch_lcd.h"
#include "watch_runtime.h"

#define WATCH_POWER_RTC_WAKE_SECONDS 3U
#define WATCH_POWER_IWDG_RELOAD 2047U

extern void SystemClock_Config(void);

static watch_power_state_t s_power_state;
static watch_watchdog_t s_watchdog;
static bool s_initialized;
static bool s_rtc_wakeup_configured;
static volatile watch_power_wake_source_t s_pending_wake_source;
static uint32_t s_stop_count;
static uint32_t s_wake_count;

static bool watch_power_watchdog_refresh(void *context)
{
    (void)context;
    IWDG->KR = 0xAAAAU;
    return true;
}

static void watch_power_watchdog_start(void)
{
    IWDG->KR = 0x5555U;
    IWDG->PR = IWDG_PR_PR_0 | IWDG_PR_PR_1 | IWDG_PR_PR_2;
    IWDG->RLR = WATCH_POWER_IWDG_RELOAD;
    IWDG->KR = 0xAAAAU;
    IWDG->KR = 0xCCCCU;
}

static bool watch_power_services_healthy(uint32_t now_ms)
{
    watch_runtime_health_t health;
    watch_runtime_service_t service;

    for (service = WATCH_RUNTIME_SERVICE_APP; service < WATCH_RUNTIME_SERVICE_COUNT;
         service = (watch_runtime_service_t)(service + 1)) {
        if (!watch_runtime_read_health(service, now_ms, &health)
            || health.state != WATCH_RUNTIME_HEALTH_HEALTHY) {
            return false;
        }
    }

    return true;
}

static void watch_power_rtc_wakeup_cancel(void)
{
    if (s_rtc_wakeup_configured) {
        (void)HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
        s_rtc_wakeup_configured = false;
    }

    __HAL_RTC_WAKEUPTIMER_EXTI_DISABLE_IT();
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
}

static bool watch_power_rtc_wakeup_schedule(void)
{
    HAL_StatusTypeDef result;

    watch_power_rtc_wakeup_cancel();
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    result = HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, WATCH_POWER_RTC_WAKE_SECONDS - 1U,
                                         RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
    s_rtc_wakeup_configured = result == HAL_OK;
    return s_rtc_wakeup_configured;
}

void watch_power_latch_early(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    /* Keep the watch power latch asserted before CubeMX initializes GPIO. */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio_init.Pin = POWER_EN_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(POWER_EN_GPIO_Port, POWER_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_Init(POWER_EN_GPIO_Port, &gpio_init);
}

bool watch_power_board_init(uint32_t now_ms)
{
    if (s_initialized) {
        return true;
    }

    s_pending_wake_source = WATCH_POWER_WAKE_NONE;
    s_stop_count = 0U;
    s_wake_count = 0U;
    if (!watch_power_state_init(&s_power_state)) {
        return false;
    }

    watch_power_watchdog_start();
    if (!watch_watchdog_init(&s_watchdog, watch_power_watchdog_refresh, NULL, now_ms)) {
        return false;
    }

    s_initialized = true;
    return true;
}

void watch_power_board_process(uint32_t now_ms)
{
    if (!s_initialized) {
        return;
    }

    (void)watch_watchdog_process(&s_watchdog, now_ms, watch_power_services_healthy(now_ms));
}

bool watch_power_board_request_display_off(void)
{
    if (!s_initialized
        || !watch_power_state_dispatch(&s_power_state, WATCH_POWER_EVENT_DISPLAY_TIMEOUT)) {
        return false;
    }

    watch_lcd_backlight_set(0U);
    return true;
}

bool watch_power_board_request_software_off(void)
{
    if (!s_initialized
        || !watch_power_state_dispatch(&s_power_state, WATCH_POWER_EVENT_SOFTWARE_OFF)) {
        return false;
    }

    /* Keep POWER_EN asserted until its board-level polarity is verified. */
    watch_lcd_backlight_set(0U);
    return true;
}

bool watch_power_board_request_stop(void)
{
    watch_power_snapshot_t snapshot;
    watch_power_wake_source_t wake_source;

    if (!s_initialized || !watch_power_state_read(&s_power_state, &snapshot)
        || snapshot.state == WATCH_POWER_STATE_OFF || watch_lcd_dma_is_busy()
        || !watch_power_state_dispatch(&s_power_state, WATCH_POWER_EVENT_STOP_REQUEST)) {
        return false;
    }

    s_pending_wake_source = WATCH_POWER_WAKE_NONE;
    if (!watch_power_rtc_wakeup_schedule()) {
        (void)watch_power_state_dispatch(&s_power_state, WATCH_POWER_EVENT_WAKE_KEY);
        return false;
    }

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_WAKE_Pin);
    watch_lcd_backlight_set(0U);

    vTaskSuspendAll();
    HAL_SuspendTick();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    HAL_NVIC_ClearPendingIRQ(OTG_FS_IRQn);
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    HAL_ResumeTick();
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    (void)xTaskResumeAll();

    watch_power_rtc_wakeup_cancel();
    wake_source =
        s_pending_wake_source == WATCH_POWER_WAKE_KEY ? WATCH_POWER_WAKE_KEY : WATCH_POWER_WAKE_RTC;
    (void)watch_power_state_dispatch(&s_power_state,
                                     wake_source == WATCH_POWER_WAKE_RTC
                                         ? WATCH_POWER_EVENT_WAKE_RTC
                                         : WATCH_POWER_EVENT_WAKE_KEY);
    watch_lcd_backlight_on();
    s_stop_count++;
    s_wake_count++;
    return true;
}

void watch_power_board_note_wake_key(void)
{
    s_pending_wake_source = WATCH_POWER_WAKE_KEY;
}

void watch_power_board_notify_wake(watch_power_wake_source_t source)
{
    watch_power_event_t event;

    if (!s_initialized || source <= WATCH_POWER_WAKE_NONE || source >= WATCH_POWER_WAKE_COUNT) {
        return;
    }

    event =
        source == WATCH_POWER_WAKE_RTC ? WATCH_POWER_EVENT_WAKE_RTC : WATCH_POWER_EVENT_WAKE_KEY;
    (void)watch_power_state_dispatch(&s_power_state, event);
    watch_lcd_backlight_on();
}

bool watch_power_board_read_status(watch_power_board_status_t *status)
{
    watch_watchdog_status_t watchdog_status;

    if (!s_initialized || status == NULL || !watch_power_state_read(&s_power_state, &status->power)
        || !watch_watchdog_read_status(&s_watchdog, &watchdog_status)) {
        return false;
    }

    status->watchdog_enabled = watchdog_status.enabled;
    status->watchdog_refresh_count = watchdog_status.refresh_count;
    status->watchdog_blocked_count = watchdog_status.blocked_count;
    status->watchdog_refresh_failure_count = watchdog_status.refresh_failure_count;
    status->stop_count = s_stop_count;
    status->wake_count = s_wake_count;
    return true;
}

// cppcheck-suppress constParameterPointer
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *rtc_handle)
{
    if (rtc_handle != NULL && rtc_handle->Instance == RTC) {
        s_pending_wake_source = WATCH_POWER_WAKE_RTC;
    }
}

void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}
