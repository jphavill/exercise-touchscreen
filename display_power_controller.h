#pragma once

#include <Arduino.h>

struct DisplayPowerController {
  uint32_t lastInteractionMs;
  bool displaySleeping;
};

void display_power_init(DisplayPowerController& controller);
void display_power_mark_activity(DisplayPowerController& controller);
void display_power_handle_inactivity(DisplayPowerController& controller, uint32_t nowMs,
                                     uint32_t inactivityTimeoutMs);
