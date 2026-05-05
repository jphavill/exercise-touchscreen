#pragma once

#include "models.h"

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

struct WorkoutScreen {
  lv_obj_t* root = nullptr;
  lv_obj_t* backButton = nullptr;
  lv_obj_t* titleLabel = nullptr;
  lv_obj_t* mainGrid = nullptr;
  lv_obj_t* timerLabel = nullptr;
  lv_obj_t* doneButton = nullptr;
  lv_obj_t* doneButtonLabel = nullptr;
  lv_obj_t* currentTitleLabel = nullptr;
  lv_obj_t* currentMetaLabel = nullptr;
  lv_obj_t* nextTitleLabel = nullptr;
  lv_obj_t* nextMetaLabel = nullptr;
  lv_obj_t* finishedLabel = nullptr;
  lv_obj_t* finishedMetaLabel = nullptr;
  const WorkoutRoutine* routine = nullptr;
  int16_t currentStepIndex = -1;
  int16_t remainingSeconds = 0;
  bool isIntroCountdown = false;
  bool isRunningTimedStep = false;
  bool isFinished = false;
  lv_timer_t* tickTimer = nullptr;
};

WorkoutScreen workout_screen_create(lv_event_cb_t backCallback);
void workout_screen_set_routine(WorkoutScreen& screen, const WorkoutRoutine& routine);
void workout_screen_start(WorkoutScreen& screen);
void workout_screen_stop(WorkoutScreen& screen);
void workout_screen_advance(WorkoutScreen& screen);
bool workout_screen_is_running(const WorkoutScreen& screen);
void workout_screen_render_intro_countdown(WorkoutScreen& screen);
void workout_screen_render_current_step(WorkoutScreen& screen);
void workout_screen_render_finished(WorkoutScreen& screen);
