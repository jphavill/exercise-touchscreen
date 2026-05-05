#include "workout_selection_screen.h"

#include "ui_style.h"

static lv_coord_t kWorkoutCols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                    LV_GRID_TEMPLATE_LAST};
static lv_coord_t kWorkoutRows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

static workout_selected_callback_t s_selectedCallback = nullptr;

static void on_workout_card(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const auto* routine = static_cast<const WorkoutRoutine*>(lv_event_get_user_data(e));
  if (routine == nullptr || s_selectedCallback == nullptr) {
    return;
  }

  s_selectedCallback(*routine);
}

WorkoutSelectionScreen workout_selection_screen_create(
    const WorkoutRoutine* routines,
    uint8_t routineCount,
    lv_event_cb_t gestureCallback,
    workout_selected_callback_t selectedCallback) {
  WorkoutSelectionScreen screen;
  s_selectedCallback = selectedCallback;

  screen.root = lv_obj_create(nullptr);
  ui_apply_base_screen_style(screen.root);
  lv_obj_set_style_pad_all(screen.root, kScreenPad, 0);
  lv_obj_set_layout(screen.root, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(screen.root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(screen.root, kSectionGap, 0);
  ui_register_gesture_screen(screen.root, gestureCallback);

  lv_obj_t* workoutSelectionTitle = lv_label_create(screen.root);
  lv_label_set_text(workoutSelectionTitle, "Workouts");
  lv_obj_set_style_text_color(workoutSelectionTitle, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(workoutSelectionTitle, UI_FONT_TITLE, 0);

  lv_obj_t* workoutGrid = lv_obj_create(screen.root);
  lv_obj_remove_style_all(workoutGrid);
  lv_obj_set_width(workoutGrid, LV_PCT(100));
  lv_obj_set_flex_grow(workoutGrid, 1);
  lv_obj_set_layout(workoutGrid, LV_LAYOUT_GRID);
  lv_obj_set_style_grid_column_dsc_array(workoutGrid, kWorkoutCols, 0);
  lv_obj_set_style_grid_row_dsc_array(workoutGrid, kWorkoutRows, 0);
  lv_obj_set_style_pad_row(workoutGrid, kGridGap, 0);
  lv_obj_set_style_pad_column(workoutGrid, kGridGap, 0);
  ui_register_tap_target(workoutGrid);

  const uint8_t renderCount =
      routines == nullptr ? 0 : (routineCount > kMaxWorkouts ? kMaxWorkouts : routineCount);
  if (renderCount == 0) {
    lv_obj_t* emptyLabel = lv_label_create(workoutGrid);
    lv_label_set_text(emptyLabel, "No workouts available");
    lv_obj_set_style_text_color(emptyLabel, lv_color_hex(0xB9C8D8), 0);
    lv_obj_set_style_text_font(emptyLabel, UI_FONT_META, 0);
    lv_obj_set_style_text_align(emptyLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(emptyLabel, LV_GRID_ALIGN_CENTER, 0, 3, LV_GRID_ALIGN_CENTER, 0, 2);
    ui_register_tap_target(emptyLabel);
    return screen;
  }

  for (uint8_t i = 0; i < renderCount; i++) {
    const WorkoutRoutine& routine = routines[i];
    lv_obj_t* card = lv_obj_create(workoutGrid);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x78C7FF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x78C7FF), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_40, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);

    lv_obj_add_event_cb(card, on_workout_card, LV_EVENT_CLICKED,
                        const_cast<WorkoutRoutine*>(&routines[i]));
    ui_register_tap_target(card);

    lv_obj_t* cardTitle = lv_label_create(card);
    lv_label_set_text(cardTitle, routine.name);
    lv_obj_set_style_text_color(cardTitle, lv_color_hex(0xF2F7FF), 0);
    lv_obj_set_style_text_font(cardTitle, UI_FONT_META, 0);

    char durationBuf[20];
    snprintf(durationBuf, sizeof(durationBuf), "%u min", static_cast<unsigned>(routine.durationMin));
    lv_obj_t* cardMeta = lv_label_create(card);
    lv_label_set_text(cardMeta, durationBuf);
    lv_obj_set_style_text_color(cardMeta, lv_color_hex(0xB9C8D8), 0);
    lv_obj_set_style_text_font(cardMeta, UI_FONT_BUTTON, 0);
  }

  return screen;
}
