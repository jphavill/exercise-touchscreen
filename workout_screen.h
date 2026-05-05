#pragma once

#include "workout_selection_screen.h"

struct WorkoutScreen {
  lv_obj_t* root = nullptr;
  lv_obj_t* titleLabel = nullptr;
  lv_obj_t* idLabel = nullptr;
};

WorkoutScreen workout_screen_create(lv_event_cb_t backCallback);

void workout_screen_set_routine(WorkoutScreen& screen, const WorkoutRoutine& routine);
