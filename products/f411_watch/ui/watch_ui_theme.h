/**
 * @file watch_ui_theme.h
 * @brief User-owned Magic Watch visual tokens and page decoration.
 */

#ifndef WATCH_UI_THEME_H
#define WATCH_UI_THEME_H

#include <stdbool.h>

#include "../core/watch_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

typedef enum {
    WATCH_UI_THEME_DARK = 0,
    WATCH_UI_THEME_LIGHT,
    WATCH_UI_THEME_COUNT
} watch_ui_theme_mode_t;

typedef struct
{
    lv_color_t background;
    lv_color_t surface;
    lv_color_t surface_alt;
    lv_color_t accent;
    lv_color_t text;
    lv_color_t muted;
    lv_color_t border;
    lv_color_t selected;
    lv_color_t success;
    lv_color_t degraded;
} watch_ui_palette_t;

const watch_ui_palette_t *watch_ui_theme_palette(watch_ui_theme_mode_t mode);
watch_ui_theme_mode_t watch_ui_theme_get_mode(void);
bool watch_ui_theme_set_mode(watch_ui_theme_mode_t mode);

/* Decorate one generated screen and refresh its dynamic labels. */
bool watch_ui_theme_apply(lv_obj_t *screen, const watch_snapshot_t *snapshot);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WATCH_UI_THEME_H */
