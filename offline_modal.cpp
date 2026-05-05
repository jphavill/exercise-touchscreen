#include "offline_modal.h"

#include "ui_style.h"

static lv_obj_t* s_offlineModal = nullptr;
static ui_action_callback_t s_refreshCallback = nullptr;

static void on_refresh_btn(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  if (s_refreshCallback != nullptr) {
    s_refreshCallback();
  }
}

static lv_obj_t* create_offline_modal() {
  lv_obj_t* modal = lv_obj_create(lv_layer_top());
  lv_obj_add_flag(modal, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_size(modal, lv_disp_get_hor_res(nullptr), lv_disp_get_ver_res(nullptr));
  lv_obj_set_pos(modal, 0, 0);
  lv_obj_set_style_bg_color(modal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(modal, LV_OPA_50, 0);
  lv_obj_set_style_border_width(modal, 0, 0);
  lv_obj_set_style_pad_all(modal, 0, 0);
  ui_register_tap_target(modal);
  lv_obj_clear_flag(modal, LV_OBJ_FLAG_GESTURE_BUBBLE);

  lv_obj_t* popup = lv_obj_create(modal);
  lv_obj_set_size(popup, 360, 220);
  lv_obj_center(popup);
  lv_obj_set_style_bg_color(popup, lv_color_hex(0x07111A), 0);
  lv_obj_set_style_bg_opa(popup, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_border_opa(popup, LV_OPA_40, 0);
  lv_obj_set_style_border_width(popup, 1, 0);
  lv_obj_set_style_radius(popup, 20, 0);
  lv_obj_set_style_pad_all(popup, 24, 0);
  lv_obj_set_style_shadow_color(popup, lv_color_hex(0x000000), 0);
  lv_obj_set_style_shadow_opa(popup, LV_OPA_60, 0);
  lv_obj_set_style_shadow_width(popup, 28, 0);
  lv_obj_set_style_shadow_spread(popup, 2, 0);
  lv_obj_set_layout(popup, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(popup, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(popup, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(popup, 10, 0);
  ui_register_tap_target(popup);

  lv_obj_t* offlineTitle = lv_label_create(popup);
  lv_label_set_text(offlineTitle, "Offline");
  lv_obj_set_style_text_color(offlineTitle, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(offlineTitle, UI_FONT_OFFLINE_TITLE, 0);

  lv_obj_t* offlineSub = lv_label_create(popup);
  lv_label_set_text(offlineSub, "Unable to reach API");
  lv_obj_set_style_text_color(offlineSub, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(offlineSub, UI_FONT_OFFLINE_SUB, 0);

  lv_obj_t* offlineRefreshBtn = lv_btn_create(popup);
  lv_obj_set_size(offlineRefreshBtn, 160, 54);
  lv_obj_set_style_bg_color(offlineRefreshBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(offlineRefreshBtn, LV_OPA_70, 0);
  lv_obj_set_style_radius(offlineRefreshBtn, 12, 0);
  lv_obj_set_style_border_width(offlineRefreshBtn, 0, 0);
  lv_obj_add_event_cb(offlineRefreshBtn, on_refresh_btn, LV_EVENT_CLICKED, nullptr);
  ui_register_tap_target(offlineRefreshBtn);

  lv_obj_t* offlineRefreshLabel = lv_label_create(offlineRefreshBtn);
  lv_label_set_text(offlineRefreshLabel, "Refresh");
  lv_obj_set_style_text_color(offlineRefreshLabel, lv_color_hex(0x041017), 0);
  lv_obj_set_style_text_font(offlineRefreshLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(offlineRefreshLabel);

  return modal;
}

void offline_modal_show(ui_action_callback_t refreshCallback) {
  s_refreshCallback = refreshCallback;
  if (s_offlineModal != nullptr) {
    lv_obj_move_foreground(s_offlineModal);
    return;
  }

  s_offlineModal = create_offline_modal();
  if (s_offlineModal == nullptr) {
    return;
  }
  lv_obj_move_foreground(s_offlineModal);
}

void offline_modal_hide() {
  if (s_offlineModal != nullptr) {
    lv_obj_del_async(s_offlineModal);
    s_offlineModal = nullptr;
  }
}

bool offline_modal_is_visible() {
  return s_offlineModal != nullptr;
}
