/**
 * @file screen_timer_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "screen_timer_gen.h"
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

lv_obj_t * screen_timer_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_timer;
    static lv_style_t style_control;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_timer);
        lv_style_init(&style_control);

        lv_style_set_bg_opa(&style_timer, (255 * 100 / 100));
        lv_style_set_bg_color(&style_timer, BG_BASE);
        lv_style_set_bg_opa(&style_control, (255 * 100 / 100));
        lv_style_set_bg_color(&style_control, SURFACE_CARD);
        lv_style_set_border_width(&style_control, 1);
        lv_style_set_border_color(&style_control, BORDER);
        lv_style_set_radius(&style_control, RADIUS_SM);
        lv_style_set_pad_all(&style_control, 0);

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_timer_#");

        lv_obj_add_style(lv_obj_0, &style_timer, 0);
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
        lv_label_set_text(page_title, "TIMER");
        lv_obj_set_x(page_title, 16);
        lv_obj_set_y(page_title, 28);
        lv_obj_set_width(page_title, CONTENT_WIDTH);
        lv_obj_set_height(page_title, 18);
        lv_obj_set_style_text_color(page_title, ACCENT, 0);
        lv_obj_set_style_text_align(page_title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(page_title, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * timer_value = lv_label_create(lv_obj_0);
        lv_obj_set_name(timer_value, "timer_value");
        lv_label_set_text(timer_value, "00:00");
        lv_obj_set_x(timer_value, 16);
        lv_obj_set_y(timer_value, 82);
        lv_obj_set_width(timer_value, CONTENT_WIDTH);
        lv_obj_set_height(timer_value, 65);
        lv_obj_set_style_text_color(timer_value, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(timer_value, montserrat_40, 0);
        lv_obj_set_style_text_align(timer_value, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(timer_value, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * timer_state = lv_label_create(lv_obj_0);
        lv_obj_set_name(timer_state, "timer_state");
        lv_label_set_text(timer_state, "READY");
        lv_obj_set_x(timer_state, 16);
        lv_obj_set_y(timer_state, 150);
        lv_obj_set_width(timer_state, CONTENT_WIDTH);
        lv_obj_set_height(timer_state, 18);
        lv_obj_set_style_text_color(timer_state, TEXT_MUTED, 0);
        lv_obj_set_style_text_align(timer_state, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(timer_state, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * timer_start = lv_obj_create(lv_obj_0);
        lv_obj_set_name(timer_start, "timer_start");
        lv_obj_set_x(timer_start, 16);
        lv_obj_set_y(timer_start, 184);
        lv_obj_set_width(timer_start, 64);
        lv_obj_set_height(timer_start, 40);
        lv_obj_set_flag(timer_start, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(timer_start, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_flag(timer_start, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(timer_start, &style_control, 0);
        lv_obj_t * timer_start_label = lv_label_create(timer_start);
        lv_obj_set_name(timer_start_label, "timer_start_label");
        lv_label_set_text(timer_start_label, "START");
        lv_obj_set_x(timer_start_label, 0);
        lv_obj_set_y(timer_start_label, 10);
        lv_obj_set_width(timer_start_label, 64);
        lv_obj_set_height(timer_start_label, 18);
        lv_obj_set_style_text_color(timer_start_label, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(timer_start_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(timer_start_label, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * timer_pause = lv_obj_create(lv_obj_0);
        lv_obj_set_name(timer_pause, "timer_pause");
        lv_obj_set_x(timer_pause, 88);
        lv_obj_set_y(timer_pause, 184);
        lv_obj_set_width(timer_pause, 64);
        lv_obj_set_height(timer_pause, 40);
        lv_obj_set_flag(timer_pause, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(timer_pause, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_flag(timer_pause, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(timer_pause, &style_control, 0);
        lv_obj_t * timer_pause_label = lv_label_create(timer_pause);
        lv_obj_set_name(timer_pause_label, "timer_pause_label");
        lv_label_set_text(timer_pause_label, "PAUSE");
        lv_obj_set_x(timer_pause_label, 0);
        lv_obj_set_y(timer_pause_label, 10);
        lv_obj_set_width(timer_pause_label, 64);
        lv_obj_set_height(timer_pause_label, 18);
        lv_obj_set_style_text_color(timer_pause_label, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(timer_pause_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(timer_pause_label, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * timer_reset = lv_obj_create(lv_obj_0);
        lv_obj_set_name(timer_reset, "timer_reset");
        lv_obj_set_x(timer_reset, 160);
        lv_obj_set_y(timer_reset, 184);
        lv_obj_set_width(timer_reset, 64);
        lv_obj_set_height(timer_reset, 40);
        lv_obj_set_flag(timer_reset, LV_OBJ_FLAG_SCROLLABLE, false);
        lv_obj_set_flag(timer_reset, LV_OBJ_FLAG_CLICKABLE, true);
        lv_obj_set_flag(timer_reset, LV_OBJ_FLAG_IGNORE_LAYOUT, true);
        lv_obj_add_style(timer_reset, &style_control, 0);
        lv_obj_t * timer_reset_label = lv_label_create(timer_reset);
        lv_obj_set_name(timer_reset_label, "timer_reset_label");
        lv_label_set_text(timer_reset_label, "RESET");
        lv_obj_set_x(timer_reset_label, 0);
        lv_obj_set_y(timer_reset_label, 10);
        lv_obj_set_width(timer_reset_label, 64);
        lv_obj_set_height(timer_reset_label, 18);
        lv_obj_set_style_text_color(timer_reset_label, TEXT_PRIMARY, 0);
        lv_obj_set_style_text_align(timer_reset_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flag(timer_reset_label, LV_OBJ_FLAG_IGNORE_LAYOUT, true);

        lv_obj_t * page_hint = lv_label_create(lv_obj_0);
        lv_obj_set_name(page_hint, "page_hint");
        lv_label_set_text(page_hint, "TOUCH A CONTROL");
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

