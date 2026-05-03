#include "display_power_controller.h"

#include "lvgl_v8_port.h"
#include "power.h"

void display_power_init(DisplayPowerController& controller) {
  controller.lastInteractionMs = 0;
  controller.displaySleeping = false;
  controller.forcedRepaintFramesAfterWake = 0;
}

void display_power_mark_activity(DisplayPowerController& controller) {
  controller.lastInteractionMs = millis();

  if (controller.displaySleeping) {
    controller.displaySleeping = false;
    controller.forcedRepaintFramesAfterWake = 2;
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
  controller.forcedRepaintFramesAfterWake = 0;
}

void display_power_repaint_after_wake(DisplayPowerController& controller) {
  if (controller.forcedRepaintFramesAfterWake == 0) {
    return;
  }

  if (!lvgl_port_lock(-1)) {
    return;
  }

  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(lv_disp_get_default());
  lvgl_port_unlock();

  controller.forcedRepaintFramesAfterWake--;
}
