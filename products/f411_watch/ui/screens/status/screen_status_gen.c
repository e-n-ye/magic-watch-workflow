/**
 * @file screen_status_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_status_gen.h"
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

lv_obj_t * screen_status_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_status;
    static lv_style_t style_rule;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_status);
        lv_style_init(&style_rule);

        lv_style_set_bg_opa(&style_status, (255 * 100 / 100));
        lv_style_set_bg_color(&style_status, BG_BASE);
        lv_style_set_bg_opa(&style_rule, (255 * 100 / 100));
        lv_style_set_bg_color(&style_rule, ACCENT);
        lv_style_set_radius(&style_rule, 1);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_status_#");

        lv_obj_add_style(lv_obj_0, &style_status, 0);
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
        lv_label_set_text(page_title, "STATUS");
        lv_obj_set_x(page_title, 16);
        lv_obj_set_y(page_title, 28);
        lv_obj_set_width(page_title, CONTENT_WIDTH);
        lv_obj_set_height(page_title, 18);
        lv_obj_set_style_text_color(page_title, ACCENT, 0);
        lv_obj_set_style_text_align(page_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_title, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * status_rule = lv_obj_create(lv_obj_0);
        lv_obj_set_name(status_rule, "status_rule");
        lv_obj_set_x(status_rule, 16);
        lv_obj_set_y(status_rule, 56);
        lv_obj_set_width(status_rule, 208);
        lv_obj_set_height(status_rule, 2);
        lv_obj_set_flag(status_rule, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(status_rule, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(status_rule, &style_rule, 0);

        lv_obj_t * status_time = lv_label_create(lv_obj_0);
        lv_obj_set_name(status_time, "status_time");
        lv_label_set_text(status_time, "TIME  --:--:--");
        lv_obj_set_x(status_time, 16);
        lv_obj_set_y(status_time, 72);
        lv_obj_set_width(status_time, 208);
        lv_obj_set_height(status_time, 18);
        lv_obj_set_style_text_color(status_time, TEXT_PRIMARY, 0);
        lv_obj_set_flag(status_time, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * status_battery = lv_label_create(lv_obj_0);
        lv_obj_set_name(status_battery, "status_battery");
        lv_label_set_text(status_battery, "BATTERY  --");
        lv_obj_set_x(status_battery, 16);
        lv_obj_set_y(status_battery, 98);
        lv_obj_set_width(status_battery, 208);
        lv_obj_set_height(status_battery, 18);
        lv_obj_set_style_text_color(status_battery, TEXT_PRIMARY, 0);
        lv_obj_set_flag(status_battery, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * status_sensors = lv_label_create(lv_obj_0);
        lv_obj_set_name(status_sensors, "status_sensors");
        lv_label_set_text(status_sensors, "SENSORS  UNAVAILABLE");
        lv_obj_set_x(status_sensors, 16);
        lv_obj_set_y(status_sensors, 124);
        lv_obj_set_width(status_sensors, 208);
        lv_obj_set_height(status_sensors, 18);
        lv_obj_set_style_text_color(status_sensors, DEGRADED, 0);
        lv_obj_set_flag(status_sensors, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * status_storage = lv_label_create(lv_obj_0);
        lv_obj_set_name(status_storage, "status_storage");
        lv_label_set_text(status_storage, "STORAGE  UNKNOWN");
        lv_obj_set_x(status_storage, 16);
        lv_obj_set_y(status_storage, 150);
        lv_obj_set_width(status_storage, 208);
        lv_obj_set_height(status_storage, 18);
        lv_obj_set_style_text_color(status_storage, TEXT_MUTED, 0);
        lv_obj_set_flag(status_storage, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * status_input = lv_label_create(lv_obj_0);
        lv_obj_set_name(status_input, "status_input");
        lv_label_set_text(status_input, "INPUT  CST816 / ENCODER");
        lv_obj_set_x(status_input, 16);
        lv_obj_set_y(status_input, 176);
        lv_obj_set_width(status_input, 208);
        lv_obj_set_height(status_input, 18);
        lv_obj_set_style_text_color(status_input, TEXT_MUTED, 0);
        lv_obj_set_flag(status_input, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * status_reason = lv_label_create(lv_obj_0);
        lv_obj_set_name(status_reason, "status_reason");
        lv_label_set_text(status_reason, "SENSOR STATUS: PENDING");
        lv_obj_set_x(status_reason, 16);
        lv_obj_set_y(status_reason, 202);
        lv_obj_set_width(status_reason, 208);
        lv_obj_set_height(status_reason, 18);
        lv_obj_set_style_text_color(status_reason, DEGRADED, 0);
        lv_obj_set_flag(status_reason, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "BACK: RETURN");
        lv_obj_set_x(page_hint, 16);
        lv_obj_set_y(page_hint, 262);
        lv_obj_set_width(page_hint, CONTENT_WIDTH);
        lv_obj_set_height(page_hint, 8);
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

