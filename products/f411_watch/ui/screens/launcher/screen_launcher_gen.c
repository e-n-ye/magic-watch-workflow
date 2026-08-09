/**
 * @file screen_launcher_gen.c
 * @brief Generated launcher screen objects from LVGL Pro Editor.
 */

#include "screen_launcher_gen.h"

lv_obj_t *screen_launcher_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *title;
    lv_obj_t *page;
    lv_obj_t *footer;

    if (screen == NULL) {
        return NULL;
    }

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820U), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    title = lv_label_create(screen);
    page = lv_label_create(screen);
    footer = lv_label_create(screen);
    if ((title == NULL) || (page == NULL) || (footer == NULL)) {
        return screen;
    }

    lv_label_set_text(title, "MAGIC WATCH");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4F7FAU), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);

    lv_label_set_text(page, "LAUNCHER");
    lv_obj_set_style_text_color(page, lv_color_hex(0x64D2FFU), LV_PART_MAIN);
    lv_obj_align(page, LV_ALIGN_CENTER, 0, -8);

    lv_label_set_text(footer, "SELECT: STATUS");
    lv_obj_set_style_text_color(footer, lv_color_hex(0xB8C7D9U), LV_PART_MAIN);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -22);

    return screen;
}
