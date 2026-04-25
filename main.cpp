#include <Arduino.h>

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

#include <ESP_Panel_Library.h>
#include "lvgl_v8_port.h"

#include "config.h"
#include "models.h"
#include "power.h"
#include "presence.h"
#include "ui.h"
#include "wifi_api.h"

using namespace esp_panel::board;

static uint32_t g_lastApiFetchMs = 0;
static uint32_t g_lastInteractionMs = 0;
static Board* g_board = nullptr;
static bool g_displayReady = false;
static volatile bool g_refreshRequested = false;

static void refresh_from_api();

static bool init_display_and_touch() {
  g_board = new Board();
  if (!g_board) {
    return false;
  }
  g_board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
  auto lcd = g_board->getLCD();
  lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
  auto lcd_bus = lcd->getBus();
  if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
    static_cast<esp_panel::drivers::BusRGB*>(lcd_bus)
        ->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
  }
#endif
#endif

  if (!g_board->begin()) {
    return false;
  }

  lvgl_port_init(g_board->getLCD(), g_board->getTouch());
  return true;
}

static void mark_activity_and_wake() {
  g_lastInteractionMs = millis();
  backlight_on();
}

static void on_refresh_button() {
  mark_activity_and_wake();
  g_refreshRequested = true;
}

static void on_tap_wake() {
  mark_activity_and_wake();
}

static void refresh_from_api() {
  PullupDashboardData data = {};
  const bool apiOk = api_fetch_data(data) && data.valid;

  if (!lvgl_port_lock(-1)) {
    return;
  }

  if (apiOk) {
    ui_set_today(data.days[29].count);
    ui_set_goal(data.dailyGoal);
    ui_set_year(data.yearTotal);
    ui_set_heatmap(data.days);
    ui_show_dashboard();
  } else {
    ui_show_offline();
  }

  lvgl_port_unlock();
}

void setup() {
  Serial.begin(115200);

  g_displayReady = init_display_and_touch();
  power_init();
  presence_init();

  if (!g_displayReady) {
    return;
  }

  if (lvgl_port_lock(-1)) {
    ui_init();
    lvgl_port_unlock();
  }
  ui_set_refresh_callback(on_refresh_button);
  ui_set_tap_wake_callback(on_tap_wake);

  mark_activity_and_wake();

  if (!wifi_connect()) {
    if (lvgl_port_lock(-1)) {
      ui_show_offline();
      lvgl_port_unlock();
    }
  } else {
    refresh_from_api();
  }

  g_lastApiFetchMs = millis();
}

void loop() {
  if (!g_displayReady) {
    delay(1000);
    return;
  }

  const uint32_t now = millis();

  if (presence_detected()) {
    mark_activity_and_wake();
  }

  const uint32_t inactiveMs = now - g_lastInteractionMs;
  if (inactiveMs >= BACKLIGHT_OFF_TIMEOUT_MS) {
    backlight_off();
  } else if (inactiveMs >= BACKLIGHT_DIM_TIMEOUT_MS) {
    backlight_dim();
  }

  if (g_refreshRequested) {
    g_refreshRequested = false;
    g_lastApiFetchMs = now;
    refresh_from_api();
  } else if ((now - g_lastApiFetchMs) >= API_REFRESH_MS) {
    g_lastApiFetchMs = now;
    refresh_from_api();
  }

  delay(LVGL_HANDLER_INTERVAL_MS);
}
