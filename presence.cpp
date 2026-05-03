#include "presence.h"

#include <Arduino.h>

#include "config.h"

static constexpr uint32_t kPresenceDebounceMs = 120;
static bool g_rawPresenceHigh = false;
static bool g_debouncedPresenceHigh = false;
static uint32_t g_rawTransitionMs = 0;

void presence_init() {
  pinMode(PRESENCE_PIN, INPUT_PULLDOWN);

  g_rawPresenceHigh = (digitalRead(PRESENCE_PIN) == HIGH);
  g_debouncedPresenceHigh = g_rawPresenceHigh;
  g_rawTransitionMs = millis();
}

bool presence_detected() {
  const bool rawHigh = (digitalRead(PRESENCE_PIN) == HIGH);
  const uint32_t now = millis();

  if (rawHigh != g_rawPresenceHigh) {
    g_rawPresenceHigh = rawHigh;
    g_rawTransitionMs = now;
  }

  // Accept a presence state only after it has remained stable for 120 ms.
  if ((now - g_rawTransitionMs) >= kPresenceDebounceMs) {
    g_debouncedPresenceHigh = g_rawPresenceHigh;
  }

  return g_debouncedPresenceHigh;
}
