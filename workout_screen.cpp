#include "workout_screen.h"

#include <stdio.h>

#include "ui_style.h"

static lv_coord_t kWorkoutScreenCols[] = {LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_FR(1),
                                          LV_GRID_TEMPLATE_LAST};
static lv_coord_t kWorkoutScreenRows[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

static WorkoutScreen* s_activeDoneScreen = nullptr;

static const WorkoutStep* workout_screen_current_step(const WorkoutScreen& screen) {
  if (screen.routine == nullptr || screen.currentStepIndex < 0 ||
      screen.currentStepIndex >= screen.routine->stepCount) {
    return nullptr;
  }

  return &screen.routine->steps[screen.currentStepIndex];
}

static const WorkoutStep* workout_screen_next_step(const WorkoutScreen& screen) {
  if (screen.routine == nullptr) {
    return nullptr;
  }

  const int16_t nextIndex = screen.currentStepIndex + 1;
  if (nextIndex < 0 || nextIndex >= screen.routine->stepCount) {
    return nullptr;
  }

  return &screen.routine->steps[nextIndex];
}

static void workout_screen_format_step_meta(const WorkoutStep& step, char* buf, size_t bufSize) {
  switch (step.kind) {
    case WorkoutStepKind::TimedExercise:
    case WorkoutStepKind::Rest:
      snprintf(buf, bufSize, "%u sec", static_cast<unsigned>(step.durationSec));
      break;
    case WorkoutStepKind::RepExercise:
      snprintf(buf, bufSize, "%u %s", static_cast<unsigned>(step.reps), step.repUnit);
      break;
  }
}

static void workout_screen_set_hidden(lv_obj_t* obj, bool hidden) {
  if (obj == nullptr) {
    return;
  }

  if (hidden) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

static void workout_screen_update_timer_label(WorkoutScreen& screen) {
  if (screen.timerLabel == nullptr) {
    return;
  }

  char buf[8];
  snprintf(buf, sizeof(buf), "%d", static_cast<int>(screen.remainingSeconds));
  lv_label_set_text(screen.timerLabel, buf);
}

static void workout_screen_delete_timer(WorkoutScreen& screen) {
  if (screen.tickTimer != nullptr) {
    lv_timer_del(screen.tickTimer);
    screen.tickTimer = nullptr;
  }
}

static void workout_tick_cb(lv_timer_t* timer);

static void workout_screen_ensure_timer(WorkoutScreen& screen) {
  if (screen.tickTimer == nullptr) {
    screen.tickTimer = lv_timer_create(workout_tick_cb, 1000, &screen);
  }
}

static void workout_screen_show_timer_mode(WorkoutScreen& screen) {
  workout_screen_set_hidden(screen.timerLabel, false);
  workout_screen_set_hidden(screen.doneButton, true);
  workout_screen_set_hidden(screen.finishedLabel, true);
  workout_screen_set_hidden(screen.finishedMetaLabel, true);
  workout_screen_set_hidden(screen.currentTitleLabel, false);
  workout_screen_set_hidden(screen.currentMetaLabel, false);
  workout_screen_set_hidden(screen.nextTitleLabel, false);
  workout_screen_set_hidden(screen.nextMetaLabel, false);
}

static void workout_screen_show_done_mode(WorkoutScreen& screen) {
  workout_screen_set_hidden(screen.timerLabel, true);
  workout_screen_set_hidden(screen.doneButton, false);
  workout_screen_set_hidden(screen.finishedLabel, true);
  workout_screen_set_hidden(screen.finishedMetaLabel, true);
  workout_screen_set_hidden(screen.currentTitleLabel, false);
  workout_screen_set_hidden(screen.currentMetaLabel, false);
  workout_screen_set_hidden(screen.nextTitleLabel, false);
  workout_screen_set_hidden(screen.nextMetaLabel, false);
}

static void workout_screen_render_step_text(WorkoutScreen& screen) {
  const WorkoutStep* current = workout_screen_current_step(screen);
  if (current == nullptr) {
    return;
  }

  char metaBuf[32];
  workout_screen_format_step_meta(*current, metaBuf, sizeof(metaBuf));
  lv_label_set_text(screen.currentTitleLabel, current->title);
  lv_label_set_text(screen.currentMetaLabel, metaBuf);

  const WorkoutStep* next = workout_screen_next_step(screen);
  if (next == nullptr) {
    lv_label_set_text(screen.nextTitleLabel, "");
    lv_label_set_text(screen.nextMetaLabel, "");
    return;
  }

  char nextMetaBuf[32];
  workout_screen_format_step_meta(*next, nextMetaBuf, sizeof(nextMetaBuf));
  lv_label_set_text(screen.nextTitleLabel, next->title);
  lv_label_set_text(screen.nextMetaLabel, nextMetaBuf);
}

static void workout_screen_start_current_step(WorkoutScreen& screen) {
  const WorkoutStep* step = workout_screen_current_step(screen);
  if (step == nullptr) {
    workout_screen_render_finished(screen);
    return;
  }

  screen.isIntroCountdown = false;
  screen.isFinished = false;

  if (step->kind == WorkoutStepKind::RepExercise) {
    screen.remainingSeconds = 0;
    screen.isRunningTimedStep = false;
    workout_screen_delete_timer(screen);
    workout_screen_render_current_step(screen);
    return;
  }

  screen.remainingSeconds = step->durationSec;
  screen.isRunningTimedStep = true;
  workout_screen_render_current_step(screen);
  workout_screen_ensure_timer(screen);
}

static void on_done_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_activeDoneScreen == nullptr) {
    return;
  }

  workout_screen_advance(*s_activeDoneScreen);
}

WorkoutScreen workout_screen_create(lv_event_cb_t backCallback) {
  WorkoutScreen screen;

  screen.root = lv_obj_create(nullptr);
  ui_apply_base_screen_style(screen.root);
  lv_obj_set_style_pad_all(screen.root, kScreenPad, 0);
  ui_register_tap_target(screen.root);

  screen.backButton = lv_btn_create(screen.root);
  lv_obj_set_size(screen.backButton, 72, 54);
  lv_obj_set_style_bg_color(screen.backButton, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(screen.backButton, LV_OPA_30, 0);
  lv_obj_set_style_radius(screen.backButton, 12, 0);
  lv_obj_set_style_border_width(screen.backButton, 0, 0);
  lv_obj_align(screen.backButton, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_event_cb(screen.backButton, backCallback, LV_EVENT_CLICKED, nullptr);
  ui_register_tap_target(screen.backButton);

  lv_obj_t* backLabel = lv_label_create(screen.backButton);
  lv_label_set_text(backLabel, "<");
  lv_obj_set_style_text_color(backLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(backLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(backLabel);

  screen.titleLabel = lv_label_create(screen.root);
  lv_label_set_text(screen.titleLabel, "Workout");
  lv_obj_set_style_text_color(screen.titleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(screen.titleLabel, UI_FONT_TITLE, 0);
  lv_obj_set_width(screen.titleLabel, LV_PCT(70));
  lv_obj_set_style_text_align(screen.titleLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(screen.titleLabel, LV_ALIGN_TOP_MID, 0, 8);

  screen.mainGrid = lv_obj_create(screen.root);
  lv_obj_remove_style_all(screen.mainGrid);
  lv_obj_set_size(screen.mainGrid, LV_PCT(100), 340);
  lv_obj_align(screen.mainGrid, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_layout(screen.mainGrid, LV_LAYOUT_GRID);
  lv_obj_set_style_grid_column_dsc_array(screen.mainGrid, kWorkoutScreenCols, 0);
  lv_obj_set_style_grid_row_dsc_array(screen.mainGrid, kWorkoutScreenRows, 0);
  lv_obj_set_style_pad_column(screen.mainGrid, kGridGap, 0);
  ui_register_tap_target(screen.mainGrid);

  lv_obj_t* leftPanel = lv_obj_create(screen.mainGrid);
  lv_obj_remove_style_all(leftPanel);
  lv_obj_set_grid_cell(leftPanel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_layout(leftPanel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(leftPanel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(leftPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  ui_register_tap_target(leftPanel);

  screen.timerLabel = lv_label_create(leftPanel);
  lv_label_set_text(screen.timerLabel, "3");
  lv_obj_set_style_text_color(screen.timerLabel, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_text_font(screen.timerLabel, UI_FONT_TODAY, 0);

  screen.doneButton = lv_btn_create(leftPanel);
  lv_obj_set_size(screen.doneButton, 150, 86);
  lv_obj_set_style_bg_color(screen.doneButton, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(screen.doneButton, LV_OPA_80, 0);
  lv_obj_set_style_radius(screen.doneButton, 18, 0);
  lv_obj_set_style_border_width(screen.doneButton, 0, 0);
  lv_obj_add_event_cb(screen.doneButton, on_done_clicked, LV_EVENT_CLICKED, nullptr);
  ui_register_tap_target(screen.doneButton);

  screen.doneButtonLabel = lv_label_create(screen.doneButton);
  lv_label_set_text(screen.doneButtonLabel, "Done");
  lv_obj_set_style_text_color(screen.doneButtonLabel, lv_color_hex(0x06101C), 0);
  lv_obj_set_style_text_font(screen.doneButtonLabel, UI_FONT_META, 0);
  lv_obj_center(screen.doneButtonLabel);

  lv_obj_t* middlePanel = lv_obj_create(screen.mainGrid);
  lv_obj_remove_style_all(middlePanel);
  lv_obj_set_grid_cell(middlePanel, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_layout(middlePanel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(middlePanel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(middlePanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(middlePanel, 14, 0);
  ui_register_tap_target(middlePanel);

  screen.currentTitleLabel = lv_label_create(middlePanel);
  lv_label_set_text(screen.currentTitleLabel, "Ready");
  lv_obj_set_width(screen.currentTitleLabel, LV_PCT(100));
  lv_obj_set_style_text_align(screen.currentTitleLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(screen.currentTitleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(screen.currentTitleLabel, UI_FONT_TITLE, 0);

  screen.currentMetaLabel = lv_label_create(middlePanel);
  lv_label_set_text(screen.currentMetaLabel, "");
  lv_obj_set_width(screen.currentMetaLabel, LV_PCT(100));
  lv_obj_set_style_text_align(screen.currentMetaLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(screen.currentMetaLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(screen.currentMetaLabel, UI_FONT_META, 0);

  screen.finishedLabel = lv_label_create(middlePanel);
  lv_label_set_text(screen.finishedLabel, "Finished");
  lv_obj_set_width(screen.finishedLabel, LV_PCT(100));
  lv_obj_set_style_text_align(screen.finishedLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(screen.finishedLabel, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_text_font(screen.finishedLabel, UI_FONT_TITLE, 0);

  screen.finishedMetaLabel = lv_label_create(middlePanel);
  lv_label_set_text(screen.finishedMetaLabel, "Workout complete");
  lv_obj_set_width(screen.finishedMetaLabel, LV_PCT(100));
  lv_obj_set_style_text_align(screen.finishedMetaLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(screen.finishedMetaLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(screen.finishedMetaLabel, UI_FONT_META, 0);

  lv_obj_t* rightPanel = lv_obj_create(screen.mainGrid);
  lv_obj_remove_style_all(rightPanel);
  lv_obj_set_grid_cell(rightPanel, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_layout(rightPanel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(rightPanel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(rightPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(rightPanel, 8, 0);
  ui_register_tap_target(rightPanel);

  screen.nextTitleLabel = lv_label_create(rightPanel);
  lv_label_set_text(screen.nextTitleLabel, "");
  lv_obj_set_width(screen.nextTitleLabel, LV_PCT(100));
  lv_obj_set_style_text_align(screen.nextTitleLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(screen.nextTitleLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(screen.nextTitleLabel, UI_FONT_META, 0);

  screen.nextMetaLabel = lv_label_create(rightPanel);
  lv_label_set_text(screen.nextMetaLabel, "");
  lv_obj_set_width(screen.nextMetaLabel, LV_PCT(100));
  lv_obj_set_style_text_align(screen.nextMetaLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(screen.nextMetaLabel, lv_color_hex(0x7A8CA0), 0);
  lv_obj_set_style_text_font(screen.nextMetaLabel, UI_FONT_BUTTON, 0);

  lv_obj_move_foreground(screen.backButton);
  workout_screen_render_finished(screen);
  return screen;
}

void workout_screen_set_routine(WorkoutScreen& screen, const WorkoutRoutine& routine) {
  workout_screen_stop(screen);
  screen.routine = &routine;
  screen.currentStepIndex = -1;
  screen.remainingSeconds = 0;
  screen.isIntroCountdown = false;
  screen.isRunningTimedStep = false;
  screen.isFinished = false;

  if (screen.titleLabel != nullptr) {
    lv_label_set_text(screen.titleLabel, routine.name);
  }
  if (screen.currentTitleLabel != nullptr) {
    lv_label_set_text(screen.currentTitleLabel, "Ready");
  }
  if (screen.currentMetaLabel != nullptr) {
    lv_label_set_text(screen.currentMetaLabel, "");
  }
  if (screen.nextTitleLabel != nullptr) {
    lv_label_set_text(screen.nextTitleLabel, "");
  }
  if (screen.nextMetaLabel != nullptr) {
    lv_label_set_text(screen.nextMetaLabel, "");
  }
  workout_screen_show_timer_mode(screen);
  workout_screen_update_timer_label(screen);
}

void workout_screen_start(WorkoutScreen& screen) {
  workout_screen_stop(screen);
  if (screen.routine == nullptr || screen.routine->stepCount == 0) {
    return;
  }

  screen.currentStepIndex = 0;
  screen.remainingSeconds = 3;
  screen.isIntroCountdown = true;
  screen.isRunningTimedStep = false;
  screen.isFinished = false;
  s_activeDoneScreen = &screen;
  workout_screen_render_intro_countdown(screen);
  workout_screen_ensure_timer(screen);
}

void workout_screen_stop(WorkoutScreen& screen) {
  workout_screen_delete_timer(screen);
  screen.isIntroCountdown = false;
  screen.isRunningTimedStep = false;
  if (s_activeDoneScreen == &screen) {
    s_activeDoneScreen = nullptr;
  }
}

void workout_screen_advance(WorkoutScreen& screen) {
  if (screen.routine == nullptr || screen.isFinished) {
    return;
  }

  screen.currentStepIndex++;
  if (screen.currentStepIndex >= screen.routine->stepCount) {
    workout_screen_render_finished(screen);
    return;
  }

  workout_screen_start_current_step(screen);
}

bool workout_screen_is_running(const WorkoutScreen& screen) {
  return screen.tickTimer != nullptr || screen.isRunningTimedStep || screen.isIntroCountdown;
}

void workout_screen_render_intro_countdown(WorkoutScreen& screen) {
  workout_screen_show_timer_mode(screen);
  workout_screen_render_step_text(screen);
  workout_screen_update_timer_label(screen);
}

void workout_screen_render_current_step(WorkoutScreen& screen) {
  const WorkoutStep* step = workout_screen_current_step(screen);
  if (step == nullptr) {
    workout_screen_render_finished(screen);
    return;
  }

  workout_screen_render_step_text(screen);
  if (step->kind == WorkoutStepKind::RepExercise) {
    workout_screen_show_done_mode(screen);
  } else {
    workout_screen_show_timer_mode(screen);
    workout_screen_update_timer_label(screen);
  }
}

void workout_screen_render_finished(WorkoutScreen& screen) {
  workout_screen_delete_timer(screen);
  screen.isIntroCountdown = false;
  screen.isRunningTimedStep = false;
  screen.isFinished = true;
  if (s_activeDoneScreen == &screen) {
    s_activeDoneScreen = nullptr;
  }

  workout_screen_set_hidden(screen.timerLabel, true);
  workout_screen_set_hidden(screen.doneButton, true);
  workout_screen_set_hidden(screen.currentTitleLabel, true);
  workout_screen_set_hidden(screen.currentMetaLabel, true);
  workout_screen_set_hidden(screen.nextTitleLabel, true);
  workout_screen_set_hidden(screen.nextMetaLabel, true);
  workout_screen_set_hidden(screen.finishedLabel, false);
  workout_screen_set_hidden(screen.finishedMetaLabel, false);
}

static void workout_tick_cb(lv_timer_t* timer) {
  auto* screen = static_cast<WorkoutScreen*>(timer->user_data);
  if (screen == nullptr || screen->isFinished) {
    return;
  }

  if (screen->remainingSeconds > 0) {
    screen->remainingSeconds--;
  }

  if (screen->remainingSeconds > 0) {
    workout_screen_update_timer_label(*screen);
    return;
  }

  if (screen->isIntroCountdown) {
    workout_screen_start_current_step(*screen);
    return;
  }

  if (screen->isRunningTimedStep) {
    workout_screen_advance(*screen);
  }
}
