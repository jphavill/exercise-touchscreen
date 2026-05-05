#pragma once

#include <Arduino.h>

#include "models.h"

bool workout_cache_refresh();
bool workout_cache_refresh_if_due(uint32_t now, uint32_t refreshIntervalMs, bool* refreshed);
bool workout_cache_has_workouts();
const WorkoutRoutine* workout_cache_get_workouts(uint8_t* count);
