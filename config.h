#pragma once

#include <Arduino.h>
#include "secrets.h"

// Network/API constants
constexpr char WIFI_SSID[] = SECRET_WIFI_SSID;
constexpr char WIFI_PASSWORD[] = SECRET_WIFI_PASSWORD;
constexpr char API_URL[] = "http://192.168.1.208:8003/api/widgets/pullups";
const uint32_t API_REFRESH_MS = 60000;

// Wi-Fi connect behavior
const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
const uint16_t WIFI_RETRY_DELAY_MS = 250;

// Inactivity/backlight behavior
const uint32_t BACKLIGHT_OFF_TIMEOUT_MS = 45000;

// Backlight pin + PWM (ESP32-S3)
const int BACKLIGHT_PIN = 2;
const uint8_t BACKLIGHT_PWM_CHANNEL = 0;
const uint16_t BACKLIGHT_PWM_FREQ = 20000;
const uint8_t BACKLIGHT_PWM_RES_BITS = 8;
const uint8_t BACKLIGHT_LEVEL_ON = 255;
const uint8_t BACKLIGHT_LEVEL_OFF = 0;

// LD2410B-P presence OUT pin (HIGH = presence)
const int PRESENCE_PIN = 4;

// Main loop cadence
const uint16_t LVGL_HANDLER_INTERVAL_MS = 5;
