#include <Arduino.h>

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

#include <ESP_Panel_Library.h>
#include "lvgl_v8_port.h"

#include "config.h"
#include "display_power_controller.h"
#include "models.h"
#include "power.h"
#include "presence.h"
#include "ui.h"
#include "wifi_api.h"

using namespace esp_panel::board;

struct RuntimeState {
  uint32_t lastApiFetchMs;
  DisplayPowerController displayPower;
};

static RuntimeState g_runtime = {};
static Board* g_board = nullptr;
static bool g_displayReady = false;

static void refresh_from_api();

static bool init_display_and_touch() {
  Serial.println("[BOOT] init_display_and_touch: start");
  g_board = new Board();
  if (!g_board) {
    Serial.println("[BOOT] Board allocation failed");
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
    Serial.println("[BOOT] board->begin() failed");
    return false;
  }

  lvgl_port_init(g_board->getLCD(), g_board->getTouch());
  Serial.println("[BOOT] init_display_and_touch: done");
  return true;
}

static void mark_activity_and_wake() {
  display_power_mark_activity(g_runtime.displayPower);
}

static void on_refresh_button() {
  mark_activity_and_wake();
  refresh_from_api();
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
    ui_set_all(data);
    ui_show_dashboard();
  } else {
    ui_show_offline();
  }

  lvgl_port_unlock();
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("[BOOT] setup start");

  g_displayReady = init_display_and_touch();
  power_init();
  presence_init();
  Serial.println("[BOOT] power + presence initialized");

  if (!g_displayReady) {
    Serial.println("[BOOT] display init failed; halting");
    return;
  }

  display_power_init(g_runtime.displayPower);

  if (lvgl_port_lock(-1)) {
    ui_init();
    lvgl_port_unlock();
  }
  ui_set_refresh_callback(on_refresh_button);
  ui_set_tap_wake_callback(on_tap_wake);

  mark_activity_and_wake();

  if (!wifi_connect()) {
    Serial.println("[BOOT] wifi connect failed");
    if (lvgl_port_lock(-1)) {
      ui_show_offline();
      lvgl_port_unlock();
    }
  } else {
    Serial.println("[BOOT] wifi connected; fetching API");
    refresh_from_api();
  }

  g_runtime.lastApiFetchMs = millis();
  Serial.println("[BOOT] setup done");
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

  display_power_repaint_after_wake(g_runtime.displayPower);
  display_power_handle_inactivity(g_runtime.displayPower, now, BACKLIGHT_OFF_TIMEOUT_MS);

  if ((now - g_runtime.lastApiFetchMs) >= API_REFRESH_MS) {
    g_runtime.lastApiFetchMs = now;
    refresh_from_api();
  }

  delay(LVGL_HANDLER_INTERVAL_MS);
}
