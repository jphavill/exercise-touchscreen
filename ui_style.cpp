#include "ui_style.h"

#if LV_FONT_MONTSERRAT_48
const lv_font_t* UI_FONT_TODAY = &lv_font_montserrat_48;
#else
const lv_font_t* UI_FONT_TODAY = LV_FONT_DEFAULT;
#endif

#if LV_FONT_MONTSERRAT_26
const lv_font_t* UI_FONT_TITLE = &lv_font_montserrat_26;
#else
const lv_font_t* UI_FONT_TITLE = LV_FONT_DEFAULT;
#endif

#if LV_FONT_MONTSERRAT_24
const lv_font_t* UI_FONT_OFFLINE_SUB = &lv_font_montserrat_24;
#else
const lv_font_t* UI_FONT_OFFLINE_SUB = LV_FONT_DEFAULT;
#endif

#if LV_FONT_MONTSERRAT_42
const lv_font_t* UI_FONT_OFFLINE_TITLE = &lv_font_montserrat_42;
#else
const lv_font_t* UI_FONT_OFFLINE_TITLE = LV_FONT_DEFAULT;
#endif

#if LV_FONT_MONTSERRAT_22
const lv_font_t* UI_FONT_META = &lv_font_montserrat_22;
#else
const lv_font_t* UI_FONT_META = LV_FONT_DEFAULT;
#endif

#if LV_FONT_MONTSERRAT_20
const lv_font_t* UI_FONT_BUTTON = &lv_font_montserrat_20;
#else
const lv_font_t* UI_FONT_BUTTON = LV_FONT_DEFAULT;
#endif

void ui_apply_base_screen_style(lv_obj_t* obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
}

void ui_register_tap_target(lv_obj_t* obj) {
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

  if (lv_obj_get_parent(obj) != nullptr) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
  }
}

void ui_register_gesture_screen(lv_obj_t* obj, lv_event_cb_t callback) {
  lv_obj_add_event_cb(obj, callback, LV_EVENT_GESTURE, nullptr);
  ui_register_tap_target(obj);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

lv_color_t ui_heat_color(uint8_t level) {
  if (level == 0) {
    return lv_color_hex(0xFFFFFF);
  }
  return lv_color_hex(0x78C7FF);
}

lv_opa_t ui_heat_opa(uint8_t level) {
  switch (level) {
    case 0:
      return static_cast<lv_opa_t>(15);   // ~6%
    case 1:
      return static_cast<lv_opa_t>(56);   // ~22%
    case 2:
      return static_cast<lv_opa_t>(115);  // ~45%
    case 3:
      return static_cast<lv_opa_t>(173);  // ~68%
    default:
      return static_cast<lv_opa_t>(235);  // ~92%
  }
}
