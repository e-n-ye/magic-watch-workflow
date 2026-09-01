/**
 * @file screen_watchface_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_watchface_gen.h"
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

lv_obj_t * screen_watchface_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_watchface;
    static lv_style_t style_rule;
    static lv_style_t style_summary;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_watchface);
        lv_style_init(&style_rule);
        lv_style_init(&style_summary);

        lv_style_set_bg_opa(&style_watchface, (255 * 100 / 100));
        lv_style_set_bg_color(&style_watchface, BG_BASE);
        lv_style_set_bg_opa(&style_rule, (255 * 100 / 100));
        lv_style_set_bg_color(&style_rule, BORDER);
        lv_style_set_radius(&style_rule, 1);
        lv_style_set_bg_opa(&style_summary, (255 * 100 / 100));
        lv_style_set_bg_color(&style_summary, SURFACE_CARD);
        lv_style_set_border_width(&style_summary, 1);
        lv_style_set_border_color(&style_summary, BORDER);
        lv_style_set_radius(&style_summary, 6);
        lv_style_set_pad_all(&style_summary, 0);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_watchface_#");

        lv_obj_add_style(lv_obj_0, &style_watchface, 0);
        lv_obj_t * page_brand = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_brand, "page_brand");
        lv_label_set_text(page_brand, "MAGIC WATCH");
        lv_obj_set_align(page_brand, LV_ALIGN_TOP_MID);
        lv_obj_set_y(page_brand, 8);
        lv_obj_set_width(page_brand, 208);
        lv_obj_set_style_text_color(page_brand, TEXT_MUTED, 0);
        lv_obj_set_style_text_align(page_brand, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_brand, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_top_rule = lv_obj_create(lv_obj_0);
        lv_obj_set_name(watchface_top_rule, "watchface_top_rule");
        lv_obj_set_x(watchface_top_rule, 16);
        lv_obj_set_y(watchface_top_rule, 32);
        lv_obj_set_width(watchface_top_rule, 208);
        lv_obj_set_height(watchface_top_rule, 1);
        lv_obj_set_flag(watchface_top_rule, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(watchface_top_rule, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(watchface_top_rule, &style_rule, 0);

        lv_obj_t * watchface_battery = lv_label_create(lv_obj_0);
        lv_obj_set_name(watchface_battery, "watchface_battery");
        lv_label_set_text(watchface_battery, "BATTERY 85%");
        lv_obj_set_x(watchface_battery, 16);
        lv_obj_set_y(watchface_battery, 40);
        lv_obj_set_width(watchface_battery, 96);
        lv_obj_set_style_text_color(watchface_battery, ACCENT, 0);
        lv_obj_set_flag(watchface_battery, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_status = lv_label_create(lv_obj_0);
        lv_obj_set_name(watchface_status, "watchface_status");
        lv_label_set_text(watchface_status, "READY");
        lv_obj_set_x(watchface_status, 112);
        lv_obj_set_y(watchface_status, 40);
        lv_obj_set_width(watchface_status, 112);
        lv_obj_set_style_text_color(watchface_status, ACCENT, 0);
        lv_obj_set_style_text_align(watchface_status, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_flag(watchface_status, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_title = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_title, "page_title");
        lv_label_set_text(page_title, "10:09");
        lv_obj_set_align(page_title, LV_ALIGN_TOP_MID);
        lv_obj_set_y(page_title, 96);
        lv_obj_set_width(page_title, 218);
        lv_obj_set_style_text_color(page_title, TEXT_PRIMARY, 0);
        lv_obj_set_height(page_title, 65);
        lv_obj_set_x(page_title, -1);
        lv_obj_set_style_text_font(page_title, montserrat_40, 0);
        lv_obj_set_style_text_align(page_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_title, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_time_rule = lv_obj_create(lv_obj_0);
        lv_obj_set_name(watchface_time_rule, "watchface_time_rule");
        lv_obj_set_x(watchface_time_rule, 18);
        lv_obj_set_y(watchface_time_rule, 184);
        lv_obj_set_width(watchface_time_rule, 208);
        lv_obj_set_height(watchface_time_rule, 10);
        lv_obj_set_flag(watchface_time_rule, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(watchface_time_rule, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(watchface_time_rule, &style_rule, 0);

        lv_obj_t * watchface_weekday = lv_label_create(lv_obj_0);
        lv_obj_set_name(watchface_weekday, "watchface_weekday");
        lv_label_set_text(watchface_weekday, "WEDNESDAY");
        lv_obj_set_x(watchface_weekday, 14);
        lv_obj_set_y(watchface_weekday, 195);
        lv_obj_set_width(watchface_weekday, 108);
        lv_obj_set_style_text_color(watchface_weekday, ACCENT, 0);
        lv_obj_set_style_text_align(watchface_weekday, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_flag(watchface_weekday, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_date = lv_label_create(lv_obj_0);
        lv_obj_set_name(watchface_date, "watchface_date");
        lv_label_set_text(watchface_date, "MAY 28");
        lv_obj_set_x(watchface_date, 133);
        lv_obj_set_y(watchface_date, 195);
        lv_obj_set_width(watchface_date, 92);
        lv_obj_set_height(watchface_date, 18);
        lv_obj_set_style_text_color(watchface_date, TEXT_PRIMARY, 0);
        lv_obj_set_flag(watchface_date, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_summary = lv_obj_create(lv_obj_0);
        lv_obj_set_name(watchface_summary, "watchface_summary");
        lv_obj_set_x(watchface_summary, 13);
        lv_obj_set_y(watchface_summary, 214);
        lv_obj_set_width(watchface_summary, 212);
        lv_obj_set_height(watchface_summary, 45);
        lv_obj_set_flag(watchface_summary, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(watchface_summary, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(watchface_summary, &style_summary, 0);
        lv_obj_t * watchface_steps = lv_label_create(watchface_summary);
        lv_obj_set_name(watchface_steps, "watchface_steps");
        lv_label_set_text(watchface_steps, "8,264");
        lv_obj_set_x(watchface_steps, 18);
        lv_obj_set_y(watchface_steps, 2);
        lv_obj_set_width(watchface_steps, 90);
        lv_obj_set_style_text_color(watchface_steps, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(watchface_steps, montserrat_24, 0);
        lv_obj_set_style_text_align(watchface_steps, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_flag(watchface_steps, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_steps_label = lv_label_create(watchface_summary);
        lv_obj_set_name(watchface_steps_label, "watchface_steps_label");
        lv_label_set_text(watchface_steps_label, "STEPS");
        lv_obj_set_x(watchface_steps_label, 117);
        lv_obj_set_y(watchface_steps_label, 9);
        lv_obj_set_width(watchface_steps_label, 72);
        lv_obj_set_style_text_color(watchface_steps_label, TEXT_MUTED, 0);
        lv_obj_set_flag(watchface_steps_label, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * watchface_summary_hint = lv_label_create(watchface_summary);
        lv_obj_set_name(watchface_summary_hint, "watchface_summary_hint");
        lv_label_set_text(watchface_summary_hint, "TAP FOR STATUS");
        lv_obj_set_x(watchface_summary_hint, 16);
        lv_obj_set_y(watchface_summary_hint, 32);
        lv_obj_set_width(watchface_summary_hint, 176);
        lv_obj_set_style_text_color(watchface_summary_hint, TEXT_MUTED, 0);
        lv_obj_set_style_text_font(watchface_summary_hint, montserrat_8, 0);
        lv_obj_set_height(watchface_summary_hint, 10);
        lv_obj_set_style_text_align(watchface_summary_hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(watchface_summary_hint, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "ENCODER PRESS  OPEN LAUNCHER");
        lv_obj_set_align(page_hint, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_y(page_hint, -10);
        lv_obj_set_width(page_hint, 208);
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

