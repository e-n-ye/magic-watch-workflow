#include "ui/watch_ui.h"

#include <stdint.h>

#include "cmsis_os.h"
#include "config/user_config.h"
#include "lvgl.h"
#include "main.h"
#include "app/watch_app.h"
#include "board/display/watch_lcd.h"
#include "watch_core.h"

#define WATCH_UI_BUFFER_LINES 20U
#define WATCH_UI_BUFFER_PIXELS (WATCH_LCD_WIDTH * WATCH_UI_BUFFER_LINES)
#define WATCH_UI_BUFFER_BYTES (WATCH_UI_BUFFER_PIXELS * 2U)
#define WATCH_UI_TASK_STACK_SIZE 4096U

static uint16_t s_draw_buffer_a[WATCH_UI_BUFFER_PIXELS];
static uint16_t s_draw_buffer_b[WATCH_UI_BUFFER_PIXELS];
static uint8_t s_dma_buffer[WATCH_UI_BUFFER_BYTES];
static osThreadId_t s_task_handle;
static lv_display_t *s_display;
static lv_obj_t *s_page_label;
static lv_obj_t *s_hint_label;
static volatile uint8_t s_dma_callback_seen;
static bool s_flush_pending;
static uint32_t s_last_tick;
static watch_snapshot_t s_last_snapshot;
static bool s_snapshot_valid;

static const char *watch_ui_page_name(watch_page_t page)
{
    static const char *const names[WATCH_PAGE_COUNT] = {
        "WATCHFACE",
        "LAUNCHER",
        "STATUS",
        "SETTINGS",
    };

    if (page >= WATCH_PAGE_COUNT) {
        return "UNKNOWN";
    }

    return names[page];
}

static const char *watch_ui_hint(const watch_snapshot_t *snapshot)
{
    if (snapshot->page == WATCH_PAGE_WATCHFACE) {
        return "SELECT: LAUNCHER";
    }

    if (snapshot->page == WATCH_PAGE_LAUNCHER) {
        return snapshot->launcher_index == 0U ? "SELECT: STATUS" : "SELECT: SETTINGS";
    }

    return "BACK: RETURN";
}

static void watch_ui_dma_done(void *context)
{
    (void)context;
    s_dma_callback_seen = 1U;
}

static void watch_ui_complete_flush(void)
{
    if (!s_flush_pending) {
        return;
    }

    if ((s_dma_callback_seen != 0U) || !watch_lcd_dma_is_busy()) {
        (void)watch_lcd_dma_consume_error();
        s_dma_callback_seen = 0U;
        s_flush_pending = false;
        lv_display_flush_ready(s_display);
    }
}

static void watch_ui_flush_wait(lv_display_t *display)
{
    (void)display;

    while (s_flush_pending) {
        watch_ui_complete_flush();
        if (s_flush_pending) {
            osDelay(1U);
        }
    }
}

// cppcheck-suppress constParameterCallback
static void watch_ui_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    int32_t width;
    int32_t height;
    uint32_t pixel_count;
    uint32_t index;
    watch_lcd_transfer_result_t result;

    if ((area == NULL) || (px_map == NULL) || (area->x1 < 0) || (area->y1 < 0)
        || (area->x2 < area->x1) || (area->y2 < area->y1)
        || (area->x2 >= (int32_t)WATCH_LCD_WIDTH) || (area->y2 >= (int32_t)WATCH_LCD_HEIGHT)) {
        lv_display_flush_ready(display);
        return;
    }

    width = area->x2 - area->x1 + 1;
    height = area->y2 - area->y1 + 1;
    pixel_count = (uint32_t)width * (uint32_t)height;
    if (pixel_count > WATCH_UI_BUFFER_PIXELS) {
        lv_display_flush_ready(display);
        return;
    }

    for (index = 0U; index < pixel_count; ++index) {
        /* LVGL stores RGB565 as a native little-endian value; ST7789 expects MSB first. */
        s_dma_buffer[index * 2U] = px_map[index * 2U + 1U];
        s_dma_buffer[index * 2U + 1U] = px_map[index * 2U];
    }

    s_dma_callback_seen = 0U;
    s_flush_pending = true;
    result = watch_lcd_draw_rgb565_bytes((uint16_t)area->x1, (uint16_t)area->y1,
                                         (uint16_t)width, (uint16_t)height, s_dma_buffer,
                                         (uint16_t)(pixel_count * 2U), watch_ui_dma_done,
                                         display);
    if (result != WATCH_LCD_TRANSFER_DMA_STARTED) {
        s_flush_pending = false;
        s_dma_callback_seen = 0U;
        (void)watch_lcd_dma_consume_error();
        lv_display_flush_ready(display);
    }
}

static void watch_ui_create_page(void)
{
    lv_obj_t *screen = lv_display_get_screen_active(s_display);
    lv_obj_t *title;

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820U), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    title = lv_label_create(screen);
    lv_label_set_text(title, "MAGIC WATCH");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FAU), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);

    s_page_label = lv_label_create(screen);
    lv_label_set_text(s_page_label, "WATCHFACE");
    lv_obj_set_style_text_color(s_page_label, lv_color_hex(0x64D2FFU), LV_PART_MAIN);
    lv_obj_align(s_page_label, LV_ALIGN_CENTER, 0, -8);

    s_hint_label = lv_label_create(screen);
    lv_label_set_text(s_hint_label, "SELECT: LAUNCHER");
    lv_obj_set_style_text_color(s_hint_label, lv_color_hex(0xB8C7D9U), LV_PART_MAIN);
    lv_obj_align(s_hint_label, LV_ALIGN_BOTTOM_MID, 0, -22);
}

static void watch_ui_update_snapshot(const watch_snapshot_t *snapshot)
{
    if ((s_page_label == NULL) || (s_hint_label == NULL) || (snapshot == NULL)) {
        return;
    }

    if (s_snapshot_valid && (s_last_snapshot.page == snapshot->page)
        && (s_last_snapshot.page_depth == snapshot->page_depth)
        && (s_last_snapshot.launcher_index == snapshot->launcher_index)
        && (s_last_snapshot.revision == snapshot->revision)) {
        return;
    }

    lv_label_set_text(s_page_label, watch_ui_page_name(snapshot->page));
    lv_label_set_text(s_hint_label, watch_ui_hint(snapshot));
    s_last_snapshot = *snapshot;
    s_snapshot_valid = true;
}

static void watch_ui_update_tick(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - s_last_tick;

    if (elapsed != 0U) {
        lv_tick_inc(elapsed);
        s_last_tick = now;
    }
}

static void watch_ui_task(void *argument)
{
    watch_snapshot_t snapshot;

    (void)argument;
    lv_init();
    s_display = lv_display_create(WATCH_LCD_WIDTH, WATCH_LCD_HEIGHT);
    if (s_display == NULL) {
        for (;;) {
            osDelay(1000U);
        }
    }

    lv_display_set_default(s_display);
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(s_display, s_draw_buffer_a, s_draw_buffer_b, sizeof(s_draw_buffer_a),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_display, watch_ui_flush);
    lv_display_set_flush_wait_cb(s_display, watch_ui_flush_wait);
    watch_ui_create_page();
    s_last_tick = HAL_GetTick();

    for (;;) {
        watch_ui_update_tick();
        watch_app_process();
        if (watch_app_read_snapshot(&snapshot)) {
            watch_ui_update_snapshot(&snapshot);
        }
        watch_ui_complete_flush();
        (void)lv_timer_handler();
        watch_ui_complete_flush();
        osDelay(5U);
    }
}

bool watch_ui_start(void)
{
    if (s_task_handle != NULL) {
        return true;
    }

    s_task_handle = osThreadNew(watch_ui_task, NULL, &(const osThreadAttr_t) {
        .name = "watchUi",
        .stack_size = WATCH_UI_TASK_STACK_SIZE,
        .priority = (osPriority_t)osPriorityNormal,
    });
    return s_task_handle != NULL;
}
