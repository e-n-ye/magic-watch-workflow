/**
 * @file screen_resources_gen.c
 * @brief Template source file for LVGL objects
 */
/*********************
 *      INCLUDES
 *********************/

#include "screen_resources_gen.h"
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

lv_obj_t * screen_resources_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_resources;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_resources);

        lv_style_set_bg_opa(&style_resources, (255 * 100 / 100));
        lv_style_set_bg_color(&style_resources, lv_color_hex(0x101820));

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if F411_WATCH_UI_CHECK_COMPILE_TARGET(F411_WATCH_UI_TARGET_ALL)
    if (f411_watch_ui_check_target(F411_WATCH_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "screen_resources_#");

        lv_obj_add_style(lv_obj_0, &style_resources, 0);
        lv_obj_t * lv_label_0 = lv_label_create(lv_obj_0);
        lv_label_set_text(lv_label_0, "MAGIC WATCH");
        lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
        lv_obj_set_y(lv_label_0, 22);
        lv_obj_set_style_text_color(lv_label_0, lv_color_hex(0xF4F7FA), 0);

        lv_obj_t * lv_label_1 = lv_label_create(lv_obj_0);
        lv_label_set_text(lv_label_1, "RESOURCES");
        lv_obj_set_align(lv_label_1, LV_ALIGN_CENTER);
        lv_obj_set_y(lv_label_1, -8);
        lv_obj_set_style_text_color(lv_label_1, lv_color_hex(0x64D2FF), 0);

        lv_obj_t * lv_label_2 = lv_label_create(lv_obj_0);
        lv_label_set_text(lv_label_2, "BACK: RETURN");
        lv_obj_set_align(lv_label_2, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_y(lv_label_2, -22);
        lv_obj_set_style_text_color(lv_label_2, lv_color_hex(0xB8C7D9), 0);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
