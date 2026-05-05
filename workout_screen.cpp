#include "workout_screen.h"

#include "ui_style.h"

WorkoutScreen workout_screen_create(lv_event_cb_t backCallback) {
  WorkoutScreen screen;

  screen.root = lv_obj_create(nullptr);
  ui_apply_base_screen_style(screen.root);
  lv_obj_set_style_pad_all(screen.root, kScreenPad, 0);
  ui_register_tap_target(screen.root);

  lv_obj_t* workoutBackBtn = lv_btn_create(screen.root);
  lv_obj_set_size(workoutBackBtn, 72, 54);
  lv_obj_set_style_bg_color(workoutBackBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(workoutBackBtn, LV_OPA_30, 0);
  lv_obj_set_style_radius(workoutBackBtn, 12, 0);
  lv_obj_set_style_border_width(workoutBackBtn, 0, 0);
  lv_obj_align(workoutBackBtn, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_event_cb(workoutBackBtn, backCallback, LV_EVENT_CLICKED, nullptr);
  ui_register_tap_target(workoutBackBtn);

  lv_obj_t* workoutBackLabel = lv_label_create(workoutBackBtn);
  lv_label_set_text(workoutBackLabel, "<");
  lv_obj_set_style_text_color(workoutBackLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(workoutBackLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(workoutBackLabel);

  lv_obj_t* workoutContent = lv_obj_create(screen.root);
  lv_obj_remove_style_all(workoutContent);
  lv_obj_set_size(workoutContent, LV_PCT(100), LV_PCT(100));
  lv_obj_set_layout(workoutContent, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(workoutContent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(workoutContent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(workoutContent, kSectionGap, 0);
  ui_register_tap_target(workoutContent);

  screen.titleLabel = lv_label_create(workoutContent);
  lv_label_set_text(screen.titleLabel, "Pull-ups");
  lv_obj_set_style_text_color(screen.titleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(screen.titleLabel, UI_FONT_TITLE, 0);

  screen.idLabel = lv_label_create(workoutContent);
  lv_label_set_text(screen.idLabel, "ID: 1");
  lv_obj_set_style_text_color(screen.idLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(screen.idLabel, UI_FONT_META, 0);
  lv_obj_move_foreground(workoutBackBtn);

  return screen;
}

void workout_screen_set_routine(WorkoutScreen& screen, const WorkoutRoutine& routine) {
  if (screen.titleLabel == nullptr || screen.idLabel == nullptr) {
    return;
  }

  lv_label_set_text(screen.titleLabel, routine.name);

  char buf[24];
  snprintf(buf, sizeof(buf), "ID: %u", static_cast<unsigned>(routine.id));
  lv_label_set_text(screen.idLabel, buf);
}
