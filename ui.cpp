#include "ui.h"

#include "dashboard_screen.h"
#include "offline_modal.h"
#include "workout_screen.h"
#include "workout_selection_screen.h"

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

static DashboardScreen s_dashboard;
static WorkoutSelectionScreen s_workoutSelection;
static WorkoutScreen s_workout;

static ui_action_callback_t s_refreshCallback = nullptr;

enum class UiPage {
  Dashboard,
  WorkoutSelection,
  Workout,
};

static UiPage s_currentPage = UiPage::Dashboard;

static WorkoutRoutine s_workoutRoutines[] = {
    {1, "Pull-ups", 10}, {2, "Push", 12},     {3, "Core", 8},
    {4, "Crimps", 15},   {5, "Mobility", 10}, {6, "Custom", 20},
};
static constexpr uint8_t kWorkoutRoutineCount =
    sizeof(s_workoutRoutines) / sizeof(s_workoutRoutines[0]);

static constexpr uint16_t kNavAnimMs = 240;

static lv_obj_t* page_screen(UiPage page) {
  switch (page) {
    case UiPage::Dashboard:
      return s_dashboard.root;
    case UiPage::WorkoutSelection:
      return s_workoutSelection.root;
    case UiPage::Workout:
      return s_workout.root;
  }
  return nullptr;
}

static void show_page(UiPage page, bool forward) {
  lv_obj_t* next = page_screen(page);
  if (next == nullptr || page == s_currentPage) {
    return;
  }

  const lv_scr_load_anim_t anim = forward ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
  lv_scr_load_anim(next, anim, kNavAnimMs, 0, false);
  s_currentPage = page;
}

static void on_refresh_action() {
  if (s_refreshCallback != nullptr) {
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

static void on_workout_selected(const WorkoutRoutine& routine) {
  workout_screen_set_routine(s_workout, routine);
  show_page(UiPage::Workout, true);
}

static void on_workout_back(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    show_page(UiPage::WorkoutSelection, false);
  }
}

void ui_init() {
  s_dashboard = dashboard_screen_create(on_dashboard_gesture, on_refresh_action);

  s_workoutSelection = workout_selection_screen_create(s_workoutRoutines, kWorkoutRoutineCount,
                                                       on_workout_selection_gesture,
                                                       on_workout_selected);

  s_workout = workout_screen_create(on_workout_back);

  lv_scr_load(s_dashboard.root);
  s_currentPage = UiPage::Dashboard;
}

void ui_show_dashboard() {
  if (s_dashboard.root == nullptr) {
    return;
  }
  ui_hide_offline();
  lv_scr_load(s_dashboard.root);
  s_currentPage = UiPage::Dashboard;
}

void ui_show_offline() {
  offline_modal_show(on_refresh_action);
}

void ui_hide_offline() {
  offline_modal_hide();
}

void ui_set_today(uint16_t todayCount) {
  dashboard_screen_set_today(s_dashboard, todayCount);
}

void ui_set_goal(uint16_t dailyGoal) {
  dashboard_screen_set_goal(s_dashboard, dailyGoal);
}

void ui_set_year(uint32_t yearTotal) {
  dashboard_screen_set_year(s_dashboard, yearTotal);
}

void ui_set_heatmap(const DayEntry days[30]) {
  dashboard_screen_set_heatmap(s_dashboard, days);
}

void ui_set_all(const PullupDashboardData& data) {
  dashboard_screen_set_all(s_dashboard, data);
}

void ui_set_refresh_callback(ui_action_callback_t callback) {
  s_refreshCallback = callback;
}
