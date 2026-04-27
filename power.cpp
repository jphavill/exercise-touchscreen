#include "power.h"

#include <Arduino.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#include "config.h"

enum class BacklightState : uint8_t {
  Unknown,
  Off,
  Dim,
  On
};

static BacklightState g_state = BacklightState::Unknown;
static bool g_pwmReady = false;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
#define USE_ESP32_LEDC_V3_API 1
#else
#define USE_ESP32_LEDC_V3_API 0
#endif

static void set_backlight(uint8_t level, BacklightState state) {
  if (g_state == state) {
    return;
  }
  if (!g_pwmReady) {
    digitalWrite(BACKLIGHT_PIN, level > 0 ? HIGH : LOW);
    g_state = state;
    return;
  }
#if USE_ESP32_LEDC_V3_API
  ledcWrite(BACKLIGHT_PIN, level);
#else
  ledcWrite(BACKLIGHT_PWM_CHANNEL, level);
#endif
  g_state = state;
}

void power_init() {
#if USE_ESP32_LEDC_V3_API
  g_pwmReady = ledcAttach(BACKLIGHT_PIN, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES_BITS);
#else
  ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES_BITS);
  ledcAttachPin(BACKLIGHT_PIN, BACKLIGHT_PWM_CHANNEL);
  g_pwmReady = true;
#endif
  if (!g_pwmReady) {
    pinMode(BACKLIGHT_PIN, OUTPUT);
    Serial.println("[BOOT] Backlight PWM attach failed; using digital fallback");
  } else {
    Serial.println("[BOOT] Backlight PWM initialized");
  }
  g_state = BacklightState::Unknown;
  set_backlight(BACKLIGHT_LEVEL_OFF, BacklightState::Off);
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
