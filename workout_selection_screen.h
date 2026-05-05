#pragma once

#include "models.h"

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

using workout_selected_callback_t = void (*)(const WorkoutRoutine& routine);

struct WorkoutSelectionScreen {
  lv_obj_t* root = nullptr;
};

WorkoutSelectionScreen workout_selection_screen_create(
    const WorkoutRoutine* routines,
    uint8_t routineCount,
    lv_event_cb_t gestureCallback,
    workout_selected_callback_t selectedCallback);
