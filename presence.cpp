#include "presence.h"

#include <Arduino.h>

#include "config.h"

void presence_init() {
  pinMode(PRESENCE_PIN, INPUT);
}

bool presence_detected() {
  return digitalRead(PRESENCE_PIN) == HIGH;
}
