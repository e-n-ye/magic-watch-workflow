/**
 * @file watch_page_lifecycle.h
 * @brief Core-driven LVGL screen creation and destruction contract.
 */

#ifndef WATCH_PAGE_LIFECYCLE_H
#define WATCH_PAGE_LIFECYCLE_H

#include <stdbool.h>
#include <stdint.h>

#include "../core/watch_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

typedef struct
{
    lv_display_t *display;
    lv_obj_t *active_screen;
    lv_obj_t *active_popup;
    watch_page_t active_page;
    uint8_t launcher_index;
    uint32_t created_count;
    uint32_t destroyed_count;
    uint32_t popup_created_count;
    uint32_t popup_destroyed_count;
    bool active;
} watch_page_lifecycle_t;

typedef struct
{
    uint32_t created_count;
    uint32_t destroyed_count;
    uint32_t popup_created_count;
    uint32_t popup_destroyed_count;
} watch_page_lifecycle_stats_t;

bool watch_page_lifecycle_init(watch_page_lifecycle_t *lifecycle, lv_display_t *display);
bool watch_page_lifecycle_apply(watch_page_lifecycle_t *lifecycle,
                                const watch_snapshot_t *snapshot);
bool watch_page_lifecycle_show_popup(watch_page_lifecycle_t *lifecycle, const char *title,
                                     const char *message);
void watch_page_lifecycle_close_popup(watch_page_lifecycle_t *lifecycle);
void watch_page_lifecycle_deinit(watch_page_lifecycle_t *lifecycle);
lv_obj_t *watch_page_lifecycle_active_screen(const watch_page_lifecycle_t *lifecycle);
lv_obj_t *watch_page_lifecycle_active_popup(const watch_page_lifecycle_t *lifecycle);
void watch_page_lifecycle_read_stats(const watch_page_lifecycle_t *lifecycle,
                                     watch_page_lifecycle_stats_t *stats);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WATCH_PAGE_LIFECYCLE_H */
