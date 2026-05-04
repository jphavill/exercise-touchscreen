#include "ui.h"

#include <Arduino.h>

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

#if LV_FONT_MONTSERRAT_48
#define UI_FONT_TODAY (&lv_font_montserrat_48)
#else
#define UI_FONT_TODAY LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_26
#define UI_FONT_TITLE (&lv_font_montserrat_26)
#else
#define UI_FONT_TITLE LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_24
#define UI_FONT_OFFLINE_SUB (&lv_font_montserrat_24)
#else
#define UI_FONT_OFFLINE_SUB LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_42
#define UI_FONT_OFFLINE_TITLE (&lv_font_montserrat_42)
#else
#define UI_FONT_OFFLINE_TITLE LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_22
#define UI_FONT_META (&lv_font_montserrat_22)
#else
#define UI_FONT_META LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_20
#define UI_FONT_BUTTON (&lv_font_montserrat_20)
#else
#define UI_FONT_BUTTON LV_FONT_DEFAULT
#endif

static lv_obj_t* s_dashboardContainer = nullptr;
static lv_obj_t* s_workoutSelectionContainer = nullptr;
static lv_obj_t* s_workoutContainer = nullptr;
static lv_obj_t* s_offlineContainer = nullptr;
static lv_obj_t* s_todayLabel = nullptr;
static lv_obj_t* s_goalLabel = nullptr;
static lv_obj_t* s_yearLabel = nullptr;
static lv_obj_t* s_workoutTitleLabel = nullptr;
static lv_obj_t* s_workoutIdLabel = nullptr;
static lv_obj_t* s_refreshBtn = nullptr;
static lv_obj_t* s_heatCells[30] = {nullptr};

static ui_action_callback_t s_refreshCallback = nullptr;

struct WorkoutRoutine {
  uint16_t id;
  const char* name;
  uint16_t durationMin;
};

enum class UiPage {
  Dashboard,
  WorkoutSelection,
  Workout,
};

static UiPage s_currentPage = UiPage::Dashboard;

static constexpr WorkoutRoutine kWorkoutRoutines[] = {
    {1, "Pull-ups", 10}, {2, "Push", 12},     {3, "Core", 8},
    {4, "Crimps", 15},   {5, "Mobility", 10}, {6, "Custom", 20},
};

static constexpr lv_coord_t kHeatCellSize = 46;
static constexpr lv_coord_t kHeatGridGap = 10;
static constexpr lv_coord_t kHeatWrapPad = 8;
static constexpr uint8_t kHeatColCount = 10;
static constexpr uint8_t kHeatRowCount = 3;
static constexpr uint16_t kNavAnimMs = 250;
static constexpr lv_coord_t kScreenPad = 24;
static constexpr lv_coord_t kGridGap = 16;
static constexpr lv_coord_t kSectionGap = 20;

static lv_coord_t kHeatCols[] = {kHeatCellSize, kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 kHeatCellSize, kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 kHeatCellSize, kHeatCellSize, LV_GRID_TEMPLATE_LAST};
static lv_coord_t kHeatRows[] = {kHeatCellSize, kHeatCellSize, kHeatCellSize, LV_GRID_TEMPLATE_LAST};

static lv_coord_t kWorkoutCols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                    LV_GRID_TEMPLATE_LAST};
static lv_coord_t kWorkoutRows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

static lv_obj_t* page_container(UiPage page) {
  switch (page) {
    case UiPage::Dashboard:
      return s_dashboardContainer;
    case UiPage::WorkoutSelection:
      return s_workoutSelectionContainer;
    case UiPage::Workout:
      return s_workoutContainer;
  }
  return nullptr;
}

static void show_page(UiPage page, bool forward) {
  lv_obj_t* next = page_container(page);
  lv_obj_t* current = page_container(s_currentPage);
  if (next == nullptr || next == current) {
    return;
  }

  const lv_coord_t screenWidth = lv_obj_get_width(lv_scr_act());
  const lv_coord_t startX = forward ? screenWidth : -screenWidth;

  lv_obj_add_flag(s_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_refreshBtn, LV_OBJ_FLAG_HIDDEN);
  if (page == UiPage::Dashboard) {
    lv_obj_clear_flag(s_refreshBtn, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_set_x(next, startX);
  lv_obj_clear_flag(next, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(next);
  if (page == UiPage::Dashboard) {
    lv_obj_move_foreground(s_refreshBtn);
  }

  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, next);
  lv_anim_set_values(&anim, startX, 0);
  lv_anim_set_time(&anim, kNavAnimMs);
  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_start(&anim);

  if (current != nullptr) {
    lv_obj_add_flag(current, LV_OBJ_FLAG_HIDDEN);
  }
  s_currentPage = page;
}

static lv_color_t heat_color(uint8_t level) {
  if (level == 0) {
    return lv_color_hex(0xFFFFFF);
  }
  return lv_color_hex(0x78C7FF);
}

static lv_opa_t heat_opa(uint8_t level) {
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

static void on_refresh_btn(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && s_refreshCallback != nullptr) {
    s_refreshCallback();
  }
}

static void on_dashboard_gesture(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
    return;
  }

  lv_indev_t* indev = lv_event_get_indev(e);
  if (indev == nullptr) {
    indev = lv_indev_get_act();
  }

  if (indev != nullptr && lv_indev_get_gesture_dir(indev) == LV_DIR_LEFT) {
    show_page(UiPage::WorkoutSelection, true);
    lv_indev_wait_release(indev);
  }
}

static void on_workout_selection_gesture(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
    return;
  }

  lv_indev_t* indev = lv_event_get_indev(e);
  if (indev == nullptr) {
    indev = lv_indev_get_act();
  }

  if (indev != nullptr && lv_indev_get_gesture_dir(indev) == LV_DIR_RIGHT) {
    show_page(UiPage::Dashboard, false);
    lv_indev_wait_release(indev);
  }
}

static void on_workout_card(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const WorkoutRoutine* routine = static_cast<const WorkoutRoutine*>(lv_event_get_user_data(e));
  if (routine == nullptr || s_workoutTitleLabel == nullptr || s_workoutIdLabel == nullptr) {
    return;
  }

  lv_label_set_text(s_workoutTitleLabel, routine->name);

  char buf[24];
  snprintf(buf, sizeof(buf), "ID: %u", static_cast<unsigned>(routine->id));
  lv_label_set_text(s_workoutIdLabel, buf);

  show_page(UiPage::Workout, true);
}

static void on_workout_back(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    show_page(UiPage::WorkoutSelection, false);
  }
}

static void register_tap_target(lv_obj_t* obj) {
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void register_gesture_root(lv_obj_t* obj) {
  register_tap_target(obj);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

static void apply_base_screen_style(lv_obj_t* obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
}

void ui_init() {
  lv_obj_t* scr = lv_scr_act();
  apply_base_screen_style(scr);

  register_tap_target(scr);

  s_dashboardContainer = lv_obj_create(scr);
  lv_obj_remove_style_all(s_dashboardContainer);
  lv_obj_set_size(s_dashboardContainer, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(s_dashboardContainer, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_dashboardContainer, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(s_dashboardContainer, kScreenPad, 0);
  lv_obj_set_layout(s_dashboardContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(s_dashboardContainer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(s_dashboardContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_add_event_cb(s_dashboardContainer, on_dashboard_gesture, LV_EVENT_GESTURE, nullptr);
  register_gesture_root(s_dashboardContainer);

  lv_obj_t* metricsCol = lv_obj_create(s_dashboardContainer);
  lv_obj_remove_style_all(metricsCol);
  lv_obj_set_size(metricsCol, LV_PCT(44), LV_PCT(100));
  lv_obj_set_layout(metricsCol, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(metricsCol, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(metricsCol, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(metricsCol, 12, 0);
  lv_obj_add_event_cb(metricsCol, on_dashboard_gesture, LV_EVENT_GESTURE, nullptr);
  register_tap_target(metricsCol);

  lv_obj_t* titleLabel = lv_label_create(metricsCol);
  lv_label_set_text(titleLabel, "Pull-ups");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(titleLabel, UI_FONT_TITLE, 0);

  s_todayLabel = lv_label_create(metricsCol);
  lv_label_set_text(s_todayLabel, "0");
  lv_obj_set_style_text_color(s_todayLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(s_todayLabel, UI_FONT_TODAY, 0);

  s_goalLabel = lv_label_create(metricsCol);
  lv_label_set_text(s_goalLabel, "Goal 0");
  lv_obj_set_style_text_color(s_goalLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(s_goalLabel, UI_FONT_META, 0);

  s_yearLabel = lv_label_create(metricsCol);
  lv_label_set_text(s_yearLabel, "Year 0");
  lv_obj_set_style_text_color(s_yearLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(s_yearLabel, UI_FONT_META, 0);

  s_refreshBtn = lv_btn_create(scr);
  lv_obj_set_size(s_refreshBtn, 160, 54);
  lv_obj_set_style_bg_color(s_refreshBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(s_refreshBtn, LV_OPA_70, 0);
  lv_obj_set_style_radius(s_refreshBtn, 12, 0);
  lv_obj_set_style_border_width(s_refreshBtn, 0, 0);
  lv_obj_add_event_cb(s_refreshBtn, on_refresh_btn, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(s_refreshBtn, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(s_refreshBtn, LV_ALIGN_TOP_RIGHT, -10, 10);
  register_tap_target(s_refreshBtn);

  lv_obj_t* refreshLabel = lv_label_create(s_refreshBtn);
  lv_label_set_text(refreshLabel, "Refresh");
  lv_obj_set_style_text_color(refreshLabel, lv_color_hex(0x041017), 0);
  lv_obj_set_style_text_font(refreshLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(refreshLabel);

  lv_obj_t* heatmapWrap = lv_obj_create(s_dashboardContainer);
  lv_obj_remove_style_all(heatmapWrap);
  lv_obj_set_size(
      heatmapWrap,
      static_cast<lv_coord_t>(kHeatColCount * kHeatCellSize + (kHeatColCount - 1) * kHeatGridGap +
                              2 * kHeatWrapPad),
      static_cast<lv_coord_t>(kHeatRowCount * kHeatCellSize + (kHeatRowCount - 1) * kHeatGridGap +
                              2 * kHeatWrapPad));
  lv_obj_set_style_pad_all(heatmapWrap, kHeatWrapPad, 0);
  lv_obj_set_layout(heatmapWrap, LV_LAYOUT_GRID);
  lv_obj_set_style_grid_column_dsc_array(heatmapWrap, kHeatCols, 0);
  lv_obj_set_style_grid_row_dsc_array(heatmapWrap, kHeatRows, 0);
  lv_obj_set_style_pad_row(heatmapWrap, kHeatGridGap, 0);
  lv_obj_set_style_pad_column(heatmapWrap, kHeatGridGap, 0);
  lv_obj_add_event_cb(heatmapWrap, on_dashboard_gesture, LV_EVENT_GESTURE, nullptr);
  register_tap_target(heatmapWrap);

  for (uint8_t i = 0; i < 30; i++) {
    lv_obj_t* cell = lv_obj_create(heatmapWrap);
    lv_obj_remove_style_all(cell);
    lv_obj_set_size(cell, kHeatCellSize, kHeatCellSize);
    lv_obj_set_style_radius(cell, 6, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_bg_color(cell, lv_color_hex(0x78C7FF), 0);
    lv_obj_set_style_bg_opa(cell, heat_opa(0), 0);

    const uint8_t col = i % 10;
    const uint8_t row = i / 10;
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_add_event_cb(cell, on_dashboard_gesture, LV_EVENT_GESTURE, nullptr);
    register_tap_target(cell);
    s_heatCells[i] = cell;
  }

  s_workoutSelectionContainer = lv_obj_create(scr);
  lv_obj_remove_style_all(s_workoutSelectionContainer);
  lv_obj_set_size(s_workoutSelectionContainer, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(s_workoutSelectionContainer, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_workoutSelectionContainer, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(s_workoutSelectionContainer, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_pad_all(s_workoutSelectionContainer, kScreenPad, 0);
  lv_obj_set_layout(s_workoutSelectionContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(s_workoutSelectionContainer, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(s_workoutSelectionContainer, kSectionGap, 0);
  lv_obj_add_event_cb(s_workoutSelectionContainer, on_workout_selection_gesture, LV_EVENT_GESTURE,
                      nullptr);
  register_gesture_root(s_workoutSelectionContainer);

  lv_obj_t* workoutSelectionTitle = lv_label_create(s_workoutSelectionContainer);
  lv_label_set_text(workoutSelectionTitle, "Workouts");
  lv_obj_set_style_text_color(workoutSelectionTitle, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(workoutSelectionTitle, UI_FONT_TITLE, 0);

  lv_obj_t* workoutGrid = lv_obj_create(s_workoutSelectionContainer);
  lv_obj_remove_style_all(workoutGrid);
  lv_obj_set_width(workoutGrid, LV_PCT(100));
  lv_obj_set_flex_grow(workoutGrid, 1);
  lv_obj_set_layout(workoutGrid, LV_LAYOUT_GRID);
  lv_obj_set_style_grid_column_dsc_array(workoutGrid, kWorkoutCols, 0);
  lv_obj_set_style_grid_row_dsc_array(workoutGrid, kWorkoutRows, 0);
  lv_obj_set_style_pad_row(workoutGrid, kGridGap, 0);
  lv_obj_set_style_pad_column(workoutGrid, kGridGap, 0);
  lv_obj_add_event_cb(workoutGrid, on_workout_selection_gesture, LV_EVENT_GESTURE, nullptr);
  register_tap_target(workoutGrid);

  for (uint8_t i = 0; i < sizeof(kWorkoutRoutines) / sizeof(kWorkoutRoutines[0]); i++) {
    const WorkoutRoutine& routine = kWorkoutRoutines[i];
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
                        const_cast<WorkoutRoutine*>(&routine));
    lv_obj_add_event_cb(card, on_workout_selection_gesture, LV_EVENT_GESTURE, nullptr);
    register_tap_target(card);

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

  s_workoutContainer = lv_obj_create(scr);
  lv_obj_remove_style_all(s_workoutContainer);
  lv_obj_set_size(s_workoutContainer, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(s_workoutContainer, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_workoutContainer, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(s_workoutContainer, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_pad_all(s_workoutContainer, kScreenPad, 0);
  register_tap_target(s_workoutContainer);

  lv_obj_t* workoutBackBtn = lv_btn_create(s_workoutContainer);
  lv_obj_set_size(workoutBackBtn, 72, 54);
  lv_obj_set_style_bg_color(workoutBackBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(workoutBackBtn, LV_OPA_30, 0);
  lv_obj_set_style_radius(workoutBackBtn, 12, 0);
  lv_obj_set_style_border_width(workoutBackBtn, 0, 0);
  lv_obj_align(workoutBackBtn, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_event_cb(workoutBackBtn, on_workout_back, LV_EVENT_CLICKED, nullptr);
  register_tap_target(workoutBackBtn);

  lv_obj_t* workoutBackLabel = lv_label_create(workoutBackBtn);
  lv_label_set_text(workoutBackLabel, "<");
  lv_obj_set_style_text_color(workoutBackLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(workoutBackLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(workoutBackLabel);

  lv_obj_t* workoutContent = lv_obj_create(s_workoutContainer);
  lv_obj_remove_style_all(workoutContent);
  lv_obj_set_size(workoutContent, LV_PCT(100), LV_PCT(100));
  lv_obj_set_layout(workoutContent, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(workoutContent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(workoutContent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(workoutContent, kSectionGap, 0);
  register_tap_target(workoutContent);

  s_workoutTitleLabel = lv_label_create(workoutContent);
  lv_label_set_text(s_workoutTitleLabel, "Pull-ups");
  lv_obj_set_style_text_color(s_workoutTitleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(s_workoutTitleLabel, UI_FONT_TITLE, 0);

  s_workoutIdLabel = lv_label_create(workoutContent);
  lv_label_set_text(s_workoutIdLabel, "ID: 1");
  lv_obj_set_style_text_color(s_workoutIdLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(s_workoutIdLabel, UI_FONT_META, 0);
  lv_obj_move_foreground(workoutBackBtn);

  lv_obj_add_flag(s_workoutSelectionContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_workoutContainer, LV_OBJ_FLAG_HIDDEN);

  s_offlineContainer = lv_obj_create(scr);
  lv_obj_remove_style_all(s_offlineContainer);
  lv_obj_set_size(s_offlineContainer, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(s_offlineContainer, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_offlineContainer, LV_OPA_COVER, 0);
  lv_obj_set_layout(s_offlineContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(s_offlineContainer, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(s_offlineContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(s_offlineContainer, 8, 0);
  register_tap_target(s_offlineContainer);

  lv_obj_t* offlineTitle = lv_label_create(s_offlineContainer);
  lv_label_set_text(offlineTitle, "Offline");
  lv_obj_set_style_text_color(offlineTitle, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(offlineTitle, UI_FONT_OFFLINE_TITLE, 0);

  lv_obj_t* offlineRefreshBtn = lv_btn_create(s_offlineContainer);
  lv_obj_set_size(offlineRefreshBtn, 160, 54);
  lv_obj_set_style_bg_color(offlineRefreshBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(offlineRefreshBtn, LV_OPA_70, 0);
  lv_obj_set_style_radius(offlineRefreshBtn, 12, 0);
  lv_obj_set_style_border_width(offlineRefreshBtn, 0, 0);
  lv_obj_add_event_cb(offlineRefreshBtn, on_refresh_btn, LV_EVENT_CLICKED, nullptr);
  register_tap_target(offlineRefreshBtn);

  lv_obj_t* offlineRefreshLabel = lv_label_create(offlineRefreshBtn);
  lv_label_set_text(offlineRefreshLabel, "Refresh");
  lv_obj_set_style_text_color(offlineRefreshLabel, lv_color_hex(0x041017), 0);
  lv_obj_set_style_text_font(offlineRefreshLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(offlineRefreshLabel);

  lv_obj_t* offlineSub = lv_label_create(s_offlineContainer);
  lv_label_set_text(offlineSub, "Unable to reach API");
  lv_obj_set_style_text_color(offlineSub, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(offlineSub, UI_FONT_OFFLINE_SUB, 0);

  ui_show_offline();
}

void ui_show_dashboard() {
  if (s_dashboardContainer == nullptr || s_workoutSelectionContainer == nullptr ||
      s_workoutContainer == nullptr || s_offlineContainer == nullptr || s_refreshBtn == nullptr) {
    return;
  }
  lv_obj_clear_flag(s_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_workoutSelectionContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_workoutContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s_refreshBtn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_x(s_dashboardContainer, 0);
  lv_obj_move_foreground(s_refreshBtn);
  s_currentPage = UiPage::Dashboard;
}

void ui_show_offline() {
  if (s_dashboardContainer == nullptr || s_workoutSelectionContainer == nullptr ||
      s_workoutContainer == nullptr || s_offlineContainer == nullptr || s_refreshBtn == nullptr) {
    return;
  }
  lv_obj_add_flag(s_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_workoutSelectionContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_workoutContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_offlineContainer);
  lv_obj_add_flag(s_refreshBtn, LV_OBJ_FLAG_HIDDEN);
}

void ui_set_today(uint16_t todayCount) {
  if (s_todayLabel == nullptr) {
    return;
  }

  char buf[12];
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(todayCount));
  lv_label_set_text(s_todayLabel, buf);
}

void ui_set_goal(uint16_t dailyGoal) {
  if (s_goalLabel == nullptr) {
    return;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), "Goal %u", static_cast<unsigned>(dailyGoal));
  lv_label_set_text(s_goalLabel, buf);
}

void ui_set_year(uint32_t yearTotal) {
  if (s_yearLabel == nullptr) {
    return;
  }

  char buf[28];
  snprintf(buf, sizeof(buf), "Year %lu", static_cast<unsigned long>(yearTotal));
  lv_label_set_text(s_yearLabel, buf);
}

void ui_set_heatmap(const DayEntry days[30]) {
  for (uint8_t i = 0; i < 30; i++) {
    if (s_heatCells[i] == nullptr) {
      continue;
    }

    const uint8_t level = days[i].heatLevel;
    lv_obj_set_style_bg_color(s_heatCells[i], heat_color(level), 0);
    lv_obj_set_style_bg_opa(s_heatCells[i], heat_opa(level), 0);
  }
}

void ui_set_all(const PullupDashboardData& data) {
  if (!data.valid) {
    return;
  }

  ui_set_today(data.days[29].count);
  ui_set_goal(data.dailyGoal);
  ui_set_year(data.yearTotal);
  ui_set_heatmap(data.days);
}

void ui_set_refresh_callback(ui_action_callback_t callback) {
  s_refreshCallback = callback;
}
