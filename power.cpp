#include "power.h"

#include <Arduino.h>

#include "config.h"

enum class BacklightState : uint8_t {
  Off,
  Dim,
  On
};

static BacklightState g_state = BacklightState::Off;

static void set_backlight(uint8_t level, BacklightState state) {
  if (g_state == state) {
    return;
  }
  ledcWrite(BACKLIGHT_PWM_CHANNEL, level);
  g_state = state;
}

void power_init() {
  ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES_BITS);
  ledcAttachPin(BACKLIGHT_PIN, BACKLIGHT_PWM_CHANNEL);
  g_state = BacklightState::Off;
  ledcWrite(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_LEVEL_OFF);
}

void backlight_on() {
  set_backlight(BACKLIGHT_LEVEL_ON, BacklightState::On);
}

void backlight_dim() {
  set_backlight(BACKLIGHT_LEVEL_DIM, BacklightState::Dim);
}

void backlight_off() {
  set_backlight(BACKLIGHT_LEVEL_OFF, BacklightState::Off);
}
