#pragma once

#include <Arduino.h>

struct DayEntry {
  char date[11];
  uint16_t count;
  uint8_t heatLevel;
};

struct PullupDashboardData {
  uint32_t yearTotal;
  uint16_t dailyGoal;
  DayEntry days[30];
  bool valid;
};
