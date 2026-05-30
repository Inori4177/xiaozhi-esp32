#include "ui_cnc_print_status_font.h"

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);

const lv_font_t *ui_cnc_print_status_font(void)
{
    return &BUILTIN_TEXT_FONT;
}

void ui_cnc_print_status_apply_font(lv_obj_t *obj)
{
    if (obj == nullptr) {
        return;
    }
    lv_obj_set_style_text_font(obj, ui_cnc_print_status_font(), LV_PART_MAIN);
}
