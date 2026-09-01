/*******************************************************************************
 * Size: 8 px
 * Bpp: 2
 * Opts: --font /fonts/Montserrat-Medium.ttf -o /fonts/montserrat_8_data.c --size 8 --bpp 2 --format lvgl --no-compress --range 0x20-0x7f
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef MONTSERRAT_8_DATA
#define MONTSERRAT_8_DATA 1
#endif

#if MONTSERRAT_8_DATA

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x65, 0x50, 0x50,

    /* U+0022 "\"" */
    0x49, 0x20, 0x00,

    /* U+0023 "#" */
    0x04, 0x46, 0xa8, 0x21, 0x07, 0xb8, 0x22, 0x00,

    /* U+0024 "$" */
    0x04, 0x0b, 0x85, 0x80, 0xb8, 0x09, 0x9b, 0x80,
    0x40,

    /* U+0025 "%" */
    0x54, 0x42, 0x21, 0x01, 0x65, 0x02, 0x11, 0x10,
    0x90,

    /* U+0026 "&" */
    0x2a, 0x02, 0x20, 0x2c, 0x08, 0x28, 0x6a, 0x80,
    0x00,

    /* U+0027 "'" */
    0x44, 0x00,

    /* U+0028 "(" */
    0x20, 0x82, 0x08, 0x20, 0x82, 0x00,

    /* U+0029 ")" */
    0x52, 0x33, 0x32, 0x50,

    /* U+002A "*" */
    0x64, 0x78, 0x10,

    /* U+002B "+" */
    0x00, 0x02, 0x06, 0xe0, 0x20, 0x00, 0x00,

    /* U+002C "," */
    0x05, 0x40,

    /* U+002D "-" */
    0x64,

    /* U+002E "." */
    0x09,

    /* U+002F "/" */
    0x02, 0x00, 0x80, 0x80, 0x20, 0x14, 0x08, 0x02,
    0x00,

    /* U+0030 "0" */
    0x2a, 0x14, 0x38, 0x0d, 0x43, 0x2a, 0x00,

    /* U+0031 "1" */
    0xb0, 0x82, 0x08, 0x20,

    /* U+0032 "2" */
    0x6a, 0x00, 0xc0, 0x60, 0x90, 0xba, 0x00,

    /* U+0033 "3" */
    0x6b, 0x02, 0x01, 0x90, 0x08, 0xaa, 0x00,

    /* U+0034 "4" */
    0x06, 0x00, 0x80, 0x22, 0x0a, 0xb8, 0x02, 0x00,

    /* U+0035 "5" */
    0x3a, 0x14, 0x06, 0xa0, 0x09, 0x6a, 0x00,

    /* U+0036 "6" */
    0x2a, 0x14, 0x0a, 0xa1, 0x42, 0x2a, 0x40,

    /* U+0037 "7" */
    0xeb, 0x50, 0xc0, 0x50, 0x30, 0x14, 0x00,

    /* U+0038 "8" */
    0x2a, 0x14, 0x63, 0xb6, 0x03, 0x2a, 0x40,

    /* U+0039 "9" */
    0x6a, 0x20, 0x92, 0xa8, 0x09, 0x2a, 0x00,

    /* U+003A ":" */
    0x90, 0x09,

    /* U+003B ";" */
    0x90, 0x05, 0x40,

    /* U+003C "<" */
    0x00, 0x06, 0x87, 0x00, 0x18, 0x00, 0x00,

    /* U+003D "=" */
    0x6a, 0x00, 0x06, 0xa0,

    /* U+003E ">" */
    0x00, 0x0a, 0x00, 0x71, 0x90, 0x00, 0x00,

    /* U+003F "?" */
    0x6a, 0x00, 0xc0, 0x90, 0x00, 0x08, 0x00,

    /* U+0040 "@" */
    0x09, 0x64, 0x22, 0xa5, 0x88, 0x22, 0x88, 0x22,
    0x26, 0xa9, 0x0a, 0x50,

    /* U+0041 "A" */
    0x02, 0x80, 0x0a, 0x00, 0x82, 0x07, 0xac, 0x20,
    0x08,

    /* U+0042 "B" */
    0x3a, 0x82, 0x09, 0x3a, 0xc2, 0x02, 0x3a, 0x90,

    /* U+0043 "C" */
    0x1a, 0x86, 0x00, 0x80, 0x06, 0x00, 0x1a, 0x80,

    /* U+0044 "D" */
    0x3a, 0x80, 0x80, 0xc2, 0x02, 0x08, 0x0c, 0x3a,
    0x80,

    /* U+0045 "E" */
    0x3a, 0x88, 0x03, 0xa4, 0x80, 0x3a, 0x80,

    /* U+0046 "F" */
    0x3a, 0x88, 0x03, 0xa4, 0x80, 0x20, 0x00,

    /* U+0047 "G" */
    0x1a, 0x86, 0x00, 0x80, 0x16, 0x05, 0x1a, 0x90,

    /* U+0048 "H" */
    0x20, 0x22, 0x02, 0x3a, 0xa2, 0x02, 0x20, 0x20,

    /* U+0049 "I" */
    0x22, 0x22, 0x20,

    /* U+004A "J" */
    0x1b, 0x40, 0x50, 0x14, 0x09, 0x2a, 0x00,

    /* U+004B "K" */
    0x20, 0x82, 0x20, 0x3e, 0x03, 0x24, 0x20, 0x90,

    /* U+004C "L" */
    0x20, 0x08, 0x02, 0x00, 0x80, 0x3a, 0x40,

    /* U+004D "M" */
    0x30, 0x0c, 0xe0, 0xb2, 0x88, 0xc8, 0xe3, 0x20,
    0x0c,

    /* U+004E "N" */
    0x30, 0x23, 0x82, 0x22, 0x22, 0x1a, 0x20, 0x60,

    /* U+004F "O" */
    0x1a, 0x91, 0x80, 0xc8, 0x02, 0x58, 0x0c, 0x1a,
    0x90,

    /* U+0050 "P" */
    0x3a, 0x82, 0x09, 0x20, 0x83, 0xa4, 0x20, 0x00,

    /* U+0051 "Q" */
    0x1a, 0x91, 0x80, 0xc8, 0x02, 0x58, 0x0c, 0x1a,
    0x90, 0x02, 0x80,

    /* U+0052 "R" */
    0x3a, 0x82, 0x09, 0x20, 0x83, 0xb8, 0x20, 0x80,

    /* U+0053 "S" */
    0x2a, 0x14, 0x02, 0xa0, 0x06, 0x6a, 0x00,

    /* U+0054 "T" */
    0xae, 0x43, 0x00, 0xc0, 0x30, 0x0c, 0x00,

    /* U+0055 "U" */
    0x20, 0x62, 0x06, 0x20, 0x62, 0x05, 0x1a, 0x80,

    /* U+0056 "V" */
    0x20, 0x14, 0x60, 0xc0, 0xc5, 0x01, 0xa0, 0x03,
    0x40,

    /* U+0057 "W" */
    0x90, 0xc0, 0x88, 0x64, 0x83, 0x22, 0x20, 0x68,
    0xa4, 0x0c, 0x0c, 0x00,

    /* U+0058 "X" */
    0x60, 0x82, 0x60, 0x0d, 0x02, 0x60, 0x50, 0x80,

    /* U+0059 "Y" */
    0x20, 0x20, 0x22, 0x00, 0x68, 0x00, 0xc0, 0x03,
    0x00,

    /* U+005A "Z" */
    0x6b, 0xc0, 0x30, 0x08, 0x02, 0x00, 0xba, 0x80,

    /* U+005B "[" */
    0x34, 0x82, 0x08, 0x20, 0x83, 0x40,

    /* U+005C "\\" */
    0x20, 0x08, 0x01, 0x40, 0x20, 0x08, 0x00, 0x40,
    0x20,

    /* U+005D "]" */
    0xb3, 0x33, 0x33, 0xb0,

    /* U+005E "^" */
    0x08, 0x08, 0x41, 0x20,

    /* U+005F "_" */
    0x55,

    /* U+0060 "`" */
    0x14,

    /* U+0061 "a" */
    0x2a, 0x0a, 0xc8, 0x21, 0xac,

    /* U+0062 "b" */
    0x60, 0x06, 0x00, 0x7a, 0x46, 0x0c, 0x60, 0xc7,
    0xa4,

    /* U+0063 "c" */
    0x2a, 0x20, 0x08, 0x00, 0xa8,

    /* U+0064 "d" */
    0x00, 0x80, 0x22, 0xaa, 0x02, 0x80, 0x8a, 0xa0,

    /* U+0065 "e" */
    0x2a, 0x29, 0x98, 0x00, 0xa8,

    /* U+0066 "f" */
    0x28, 0x20, 0xb4, 0x20, 0x20, 0x20,

    /* U+0067 "g" */
    0x2a, 0xa0, 0x39, 0x0c, 0xaa, 0x2a, 0x40,

    /* U+0068 "h" */
    0x60, 0x18, 0x07, 0xa5, 0x82, 0x60, 0xd8, 0x30,

    /* U+0069 "i" */
    0x50, 0x66, 0x66,

    /* U+006A "j" */
    0x08, 0x00, 0x82, 0x08, 0x26, 0x40,

    /* U+006B "k" */
    0x60, 0x18, 0x06, 0x25, 0xb0, 0x76, 0x18, 0x50,

    /* U+006C "l" */
    0x66, 0x66, 0x66,

    /* U+006D "m" */
    0x7a, 0xa9, 0x60, 0x82, 0x60, 0x83, 0x60, 0x83,

    /* U+006E "n" */
    0x7a, 0x58, 0x26, 0x0d, 0x83,

    /* U+006F "o" */
    0x2a, 0x20, 0x28, 0x08, 0xa8,

    /* U+0070 "p" */
    0x7a, 0x46, 0x0c, 0x60, 0xc7, 0xa4, 0x60, 0x00,

    /* U+0071 "q" */
    0x2a, 0xa0, 0x28, 0x08, 0xaa, 0x00, 0x80,

    /* U+0072 "r" */
    0x78, 0x60, 0x60, 0x60,

    /* U+0073 "s" */
    0x69, 0x90, 0x06, 0x69,

    /* U+0074 "t" */
    0x20, 0xb4, 0x20, 0x20, 0x38,

    /* U+0075 "u" */
    0x50, 0x94, 0x26, 0x08, 0xaa,

    /* U+0076 "v" */
    0x20, 0x91, 0x48, 0x09, 0x40, 0x70,

    /* U+0077 "w" */
    0x83, 0x08, 0x56, 0x88, 0x28, 0xa0, 0x28, 0x60,

    /* U+0078 "x" */
    0x52, 0x0b, 0x02, 0x81, 0x48,

    /* U+0079 "y" */
    0x20, 0x80, 0x88, 0x0a, 0x00, 0x60, 0x28, 0x00,

    /* U+007A "z" */
    0x6a, 0x08, 0x20, 0xba,

    /* U+007B "{" */
    0x24, 0xc3, 0x18, 0x30, 0xc2, 0x40,

    /* U+007C "|" */
    0x22, 0x22, 0x22, 0x20,

    /* U+007D "}" */
    0xa0, 0x82, 0x0c, 0x20, 0x89, 0x00,

    /* U+007E "~" */
    0x21, 0x11, 0x80
};

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 34, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 34, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 50, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 6, .adv_w = 90, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 79, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 23, .adv_w = 108, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 88, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 41, .adv_w = 27, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 43, .adv_w = 43, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 49, .adv_w = 43, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 53, .adv_w = 51, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 56, .adv_w = 74, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 29, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 65, .adv_w = 49, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 66, .adv_w = 29, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 45, .box_w = 5, .box_h = 7, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 76, .adv_w = 85, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 47, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 73, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 73, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 86, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 73, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 79, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 82, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 79, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 29, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 29, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 149, .adv_w = 74, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 74, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 160, .adv_w = 74, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 73, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 132, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 186, .adv_w = 94, .box_w = 7, .box_h = 5, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 97, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 91, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 106, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 86, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 81, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 99, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 104, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 40, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 66, .box_w = 5, .box_h = 5, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 92, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 268, .adv_w = 76, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 275, .adv_w = 122, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 104, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 108, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 92, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 108, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 320, .adv_w = 93, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 79, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 75, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 101, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 91, .box_w = 7, .box_h = 5, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 359, .adv_w = 144, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 86, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 83, .box_w = 7, .box_h = 5, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 388, .adv_w = 84, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 43, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 402, .adv_w = 45, .box_w = 5, .box_h = 7, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 411, .adv_w = 43, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 415, .adv_w = 75, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 419, .adv_w = 64, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 420, .adv_w = 77, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 421, .adv_w = 77, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 87, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 73, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 87, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 448, .adv_w = 78, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 453, .adv_w = 45, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 88, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 466, .adv_w = 87, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 474, .adv_w = 36, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 36, .box_w = 3, .box_h = 7, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 483, .adv_w = 79, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 36, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 135, .box_w = 8, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 87, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 81, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 87, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 520, .adv_w = 87, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 527, .adv_w = 52, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 531, .adv_w = 64, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 53, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 87, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 72, .box_w = 6, .box_h = 4, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 551, .adv_w = 115, .box_w = 8, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 71, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 564, .adv_w = 72, .box_w = 6, .box_h = 5, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 572, .adv_w = 67, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 45, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 582, .adv_w = 38, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 586, .adv_w = 45, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 592, .adv_w = 74, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 2,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t montserrat_8_data = {
#else
lv_font_t montserrat_8_data = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 8,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if LV_VERSION_CHECK(9, 6, 0) || LVGL_VERSION_MAJOR >= 10
    .cap_height = 6,           /*Cap height of the font*/
    .x_height = 4,               /*x-height of the font*/
#endif
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif

#if LV_VERSION_CHECK(9, 3, 0)
    .static_bitmap = 1,    /*Bitmaps are stored as const so they are always static if not compressed */
#endif

    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if MONTSERRAT_8_DATA*/
