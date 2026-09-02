/**
 * @file screen_settings_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_settings_gen.h"
#include "../../f411_watch_ui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * screen_settings_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_settings;
    static lv_style_t style_list;
    static lv_style_t style_card;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_settings);
        lv_style_init(&style_list);
        lv_style_init(&style_card);

        lv_style_set_bg_opa(&style_settings, (255 * 100 / 100));
        lv_style_set_bg_color(&style_settings, BG_BASE);
        lv_style_set_bg_opa(&style_list, (255 * 0 / 100));
        lv_style_set_border_width(&style_list, 0);
        lv_style_set_radius(&style_list, 0);
        lv_style_set_pad_all(&style_list, 0);
        lv_style_set_bg_opa(&style_card, (255 * 100 / 100));
        lv_style_set_bg_color(&style_card, SURFACE_CARD);
        lv_style_set_border_width(&style_card, 1);
        lv_style_set_border_color(&style_card, BORDER);
        lv_style_set_radius(&style_card, RADIUS_SM);
        lv_style_set_pad_all(&style_card, 0);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_settings_#");

        lv_obj_add_style(lv_obj_0, &style_settings, 0);
        lv_obj_t * page_brand = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_brand, "page_brand");
        lv_label_set_text(page_brand, "MAGIC WATCH");
        lv_obj_set_x(page_brand, 16);
        lv_obj_set_y(page_brand, 8);
        lv_obj_set_width(page_brand, CONTENT_WIDTH);
        lv_obj_set_height(page_brand, 18);
        lv_obj_set_style_text_color(page_brand, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(page_brand, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_brand, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_title = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_title, "page_title");
        lv_label_set_text(page_title, "SETTINGS");
        lv_obj_set_x(page_title, 16);
        lv_obj_set_y(page_title, 28);
        lv_obj_set_width(page_title, CONTENT_WIDTH);
        lv_obj_set_height(page_title, 18);
        lv_obj_set_style_text_color(page_title, ACCENT, 0);
        lv_obj_set_style_text_align(page_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_title, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * settings_list = lv_obj_create(lv_obj_0);
        lv_obj_set_name(settings_list, "settings_list");
        lv_obj_set_x(settings_list, 16);
        lv_obj_set_y(settings_list, 58);
        lv_obj_set_width(settings_list, 208);
        lv_obj_set_height(settings_list, 150);
        lv_obj_set_flag(settings_list, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(settings_list, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(settings_list, &style_list, 0);
        lv_obj_t * settings_item_theme = lv_obj_create(settings_list);
        lv_obj_set_name(settings_item_theme, "settings_item_theme");
        lv_obj_set_x(settings_item_theme, 0);
        lv_obj_set_y(settings_item_theme, 0);
        lv_obj_set_width(settings_item_theme, 208);
        lv_obj_set_height(settings_item_theme, 40);
        lv_obj_set_flag(settings_item_theme, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(settings_item_theme, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(settings_item_theme, &style_card, 0);
        lv_obj_t * settings_title_theme = lv_label_create(settings_item_theme);
        lv_obj_set_name(settings_title_theme, "settings_title_theme");
        lv_label_set_text(settings_title_theme, "THEME");
        lv_obj_set_x(settings_title_theme, 12);
        lv_obj_set_y(settings_title_theme, 10);
        lv_obj_set_width(settings_title_theme, 128);
        lv_obj_set_height(settings_title_theme, 18);
        lv_obj_set_style_text_color(settings_title_theme, TEXT_PRIMARY, 0);
        lv_obj_set_flag(settings_title_theme, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * settings_value_theme = lv_label_create(settings_item_theme);
        lv_obj_set_name(settings_value_theme, "settings_value_theme");
        lv_label_set_text(settings_value_theme, "DARK");
        lv_obj_set_x(settings_value_theme, 142);
        lv_obj_set_y(settings_value_theme, 10);
        lv_obj_set_width(settings_value_theme, 54);
        lv_obj_set_height(settings_value_theme, 18);
        lv_obj_set_style_text_color(settings_value_theme, ACCENT, 0);
        lv_obj_set_style_text_align(settings_value_theme, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_flag(settings_value_theme, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * settings_item_brightness = lv_obj_create(settings_list);
        lv_obj_set_name(settings_item_brightness, "settings_item_brightness");
        lv_obj_set_x(settings_item_brightness, 0);
        lv_obj_set_y(settings_item_brightness, 44);
        lv_obj_set_width(settings_item_brightness, 208);
        lv_obj_set_height(settings_item_brightness, 40);
        lv_obj_set_flag(settings_item_brightness, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(settings_item_brightness, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(settings_item_brightness, &style_card, 0);
        lv_obj_t * settings_title_brightness = lv_label_create(settings_item_brightness);
        lv_obj_set_name(settings_title_brightness, "settings_title_brightness");
        lv_label_set_text(settings_title_brightness, "BRIGHTNESS");
        lv_obj_set_x(settings_title_brightness, 12);
        lv_obj_set_y(settings_title_brightness, 10);
        lv_obj_set_width(settings_title_brightness, 128);
        lv_obj_set_height(settings_title_brightness, 18);
        lv_obj_set_style_text_color(settings_title_brightness, TEXT_PRIMARY, 0);
        lv_obj_set_flag(settings_title_brightness, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * settings_value_brightness = lv_label_create(settings_item_brightness);
        lv_obj_set_name(settings_value_brightness, "settings_value_brightness");
        lv_label_set_text(settings_value_brightness, "80%");
        lv_obj_set_x(settings_value_brightness, 142);
        lv_obj_set_y(settings_value_brightness, 10);
        lv_obj_set_width(settings_value_brightness, 54);
        lv_obj_set_height(settings_value_brightness, 18);
        lv_obj_set_style_text_color(settings_value_brightness, ACCENT, 0);
        lv_obj_set_style_text_align(settings_value_brightness, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_flag(settings_value_brightness, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * settings_item_time_format = lv_obj_create(settings_list);
        lv_obj_set_name(settings_item_time_format, "settings_item_time_format");
        lv_obj_set_x(settings_item_time_format, 0);
        lv_obj_set_y(settings_item_time_format, 88);
        lv_obj_set_width(settings_item_time_format, 208);
        lv_obj_set_height(settings_item_time_format, 40);
        lv_obj_set_flag(settings_item_time_format, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(settings_item_time_format, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_add_style(settings_item_time_format, &style_card, 0);
        lv_obj_t * settings_title_time_format = lv_label_create(settings_item_time_format);
        lv_obj_set_name(settings_title_time_format, "settings_title_time_format");
        lv_label_set_text(settings_title_time_format, "TIME FORMAT");
        lv_obj_set_x(settings_title_time_format, 12);
        lv_obj_set_y(settings_title_time_format, 10);
        lv_obj_set_width(settings_title_time_format, 128);
        lv_obj_set_height(settings_title_time_format, 18);
        lv_obj_set_style_text_color(settings_title_time_format, TEXT_PRIMARY, 0);
        lv_obj_set_flag(settings_title_time_format, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * settings_value_time_format = lv_label_create(settings_item_time_format);
        lv_obj_set_name(settings_value_time_format, "settings_value_time_format");
        lv_label_set_text(settings_value_time_format, "24 H");
        lv_obj_set_x(settings_value_time_format, 142);
        lv_obj_set_y(settings_value_time_format, 10);
        lv_obj_set_width(settings_value_time_format, 54);
        lv_obj_set_height(settings_value_time_format, 18);
        lv_obj_set_style_text_color(settings_value_time_format, ACCENT, 0);
        lv_obj_set_style_text_align(settings_value_time_format, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_flag(settings_value_time_format, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "TOUCH A ROW TO CHANGE");
        lv_obj_set_x(page_hint, 16);
        lv_obj_set_y(page_hint, 262);
        lv_obj_set_width(page_hint, CONTENT_WIDTH);
        lv_obj_set_height(page_hint, 18);
        lv_obj_set_style_text_color(page_hint, TEXT_MUTED, 0);
        lv_obj_set_style_text_font(page_hint, montserrat_8, 0);
        lv_obj_set_style_text_align(page_hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_hint, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

