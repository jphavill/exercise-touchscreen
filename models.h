#pragma once

#include <Arduino.h>

struct DayEntry {
  uint16_t count;
  uint8_t heatLevel;
};

struct PullupDashboardData {
  uint32_t yearTotal;
  uint16_t dailyGoal;
  DayEntry days[30];
  bool valid;
};

enum class WorkoutStepKind : uint8_t {
  TimedExercise,
  Rest,
  RepExercise,
};

struct WorkoutStep {
  WorkoutStepKind kind;
  char title[32];
  uint16_t durationSec;
  uint16_t reps;
  char repUnit[20];
};

constexpr uint8_t kMaxWorkoutSteps = 32;

struct WorkoutRoutine {
  uint16_t id;
  char name[40];
  uint16_t durationMin;
  uint8_t stepCount;
  WorkoutStep steps[kMaxWorkoutSteps];
};

constexpr uint8_t kMaxWorkouts = 6;

struct WorkoutCache {
  WorkoutRoutine workouts[kMaxWorkouts];
  uint8_t workoutCount = 0;
  bool hasLoadedOnce = false;
  bool refreshInProgress = false;
  uint32_t lastRefreshMillis = 0;
};
