#include "workout_cache.h"

#include "wifi_api.h"

static WorkoutCache s_workoutCache;
static WorkoutCache s_workoutScratch;

bool workout_cache_refresh() {
  if (s_workoutCache.refreshInProgress) {
    return false;
  }

  const uint32_t refreshMillis = millis();
  s_workoutCache.refreshInProgress = true;
  s_workoutCache.lastRefreshMillis = refreshMillis;

  const bool ok = fetch_workouts(s_workoutScratch);
  if (ok) {
    s_workoutCache = s_workoutScratch;
    s_workoutCache.lastRefreshMillis = refreshMillis;
  }

  s_workoutCache.refreshInProgress = false;
  return ok;
}

bool workout_cache_refresh_if_due(uint32_t now, uint32_t refreshIntervalMs, bool* refreshed) {
  if (refreshed != nullptr) {
    *refreshed = false;
  }

  if (s_workoutCache.refreshInProgress || (now - s_workoutCache.lastRefreshMillis) < refreshIntervalMs) {
    return false;
  }

  if (refreshed != nullptr) {
    *refreshed = true;
  }
  return workout_cache_refresh();
}

bool workout_cache_has_workouts() {
  return s_workoutCache.hasLoadedOnce && s_workoutCache.workoutCount > 0;
}

const WorkoutRoutine* workout_cache_get_workouts(uint8_t* count) {
  if (count != nullptr) {
    *count = s_workoutCache.workoutCount;
  }

  return s_workoutCache.workouts;
}
