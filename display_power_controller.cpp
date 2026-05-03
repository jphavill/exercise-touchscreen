#include "display_power_controller.h"

#include "power.h"

void display_power_init(DisplayPowerController& controller) {
  controller.lastInteractionMs = 0;
  controller.displaySleeping = false;
}

void display_power_mark_activity(DisplayPowerController& controller) {
  controller.lastInteractionMs = millis();

  if (controller.displaySleeping) {
    controller.displaySleeping = false;
  }

  backlight_on();
}

void display_power_handle_inactivity(DisplayPowerController& controller, uint32_t nowMs,
                                     uint32_t inactivityTimeoutMs) {
  const uint32_t inactiveMs = nowMs - controller.lastInteractionMs;
  if (inactiveMs < inactivityTimeoutMs || controller.displaySleeping) {
    return;
  }

  backlight_off();
  controller.displaySleeping = true;
}
