#pragma once

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

extern const lv_font_t* UI_FONT_TODAY;
extern const lv_font_t* UI_FONT_TITLE;
extern const lv_font_t* UI_FONT_OFFLINE_SUB;
extern const lv_font_t* UI_FONT_OFFLINE_TITLE;
extern const lv_font_t* UI_FONT_META;
extern const lv_font_t* UI_FONT_BUTTON;

constexpr lv_coord_t kScreenPad = 24;
constexpr lv_coord_t kGridGap = 16;
constexpr lv_coord_t kSectionGap = 20;

void ui_apply_base_screen_style(lv_obj_t* obj);
void ui_register_tap_target(lv_obj_t* obj);
void ui_register_gesture_screen(lv_obj_t* obj, lv_event_cb_t callback);

lv_color_t ui_heat_color(uint8_t level);
lv_opa_t ui_heat_opa(uint8_t level);
