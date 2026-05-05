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
#include "workout_cache.h"

using namespace esp_panel::board;

struct RuntimeState {
  uint32_t lastDashboardFetchMs;
  DisplayPowerController displayPower;
  bool hasDashboardData;
  bool dashboardHealthy;
  bool workoutsHealthy;
};

static RuntimeState g_runtime = {};
static Board* g_board = nullptr;
static bool g_displayReady = false;
static volatile bool g_displayActivityPending = false;

static void refresh_from_api();
static void refresh_workouts_once();
static void refresh_workouts_if_idle(uint32_t now);

static void sync_offline_modal_locked() {
  if (g_runtime.dashboardHealthy && g_runtime.workoutsHealthy) {
    ui_hide_offline();
    return;
  }

  ui_show_offline();
}

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
  g_displayActivityPending = true;
  refresh_from_api();
  refresh_workouts_once();
}

static void on_tap_wake() {
  g_displayActivityPending = true;
}

static void refresh_from_api() {
  PullupDashboardData data = {};
  const bool apiOk = fetch_dashboard_data(data) && data.valid;

  if (!lvgl_port_lock(-1)) {
    return;
  }

  if (apiOk) {
    ui_set_all(data);
    if (!g_runtime.hasDashboardData) {
      ui_show_dashboard();
    }
    g_runtime.hasDashboardData = true;
  }

  g_runtime.dashboardHealthy = apiOk;
  sync_offline_modal_locked();

  lvgl_port_unlock();
}

static void push_cached_workouts_to_ui_locked() {
  uint8_t count = 0;
  const WorkoutRoutine* routines = workout_cache_get_workouts(&count);
  ui_set_workouts(routines, count);
}

static void handle_workout_refresh_result_locked(bool apiOk) {
  const bool hasCachedWorkouts = workout_cache_has_workouts();
  g_runtime.workoutsHealthy = apiOk || hasCachedWorkouts;
  if (hasCachedWorkouts) {
    push_cached_workouts_to_ui_locked();
  }
  sync_offline_modal_locked();
}

static void refresh_workouts_once() {
  const bool apiOk = workout_cache_refresh();

  if (lvgl_port_lock(-1)) {
    handle_workout_refresh_result_locked(apiOk);
    lvgl_port_unlock();
  }
}

static void refresh_workouts_if_idle(uint32_t now) {
  if (ui_is_workout_running() || ui_is_workout_page_active()) {
    return;
  }

  bool refreshed = false;
  const bool apiOk = workout_cache_refresh_if_due(now, WORKOUT_CACHE_REFRESH_MS, &refreshed);
  if (!refreshed) {
    return;
  }

  if (lvgl_port_lock(-1)) {
    handle_workout_refresh_result_locked(apiOk);
    lvgl_port_unlock();
  }
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
  lvgl_port_set_touch_activity_callback(on_tap_wake);

  mark_activity_and_wake();

  if (!wifi_connect()) {
    Serial.println("[BOOT] wifi connect failed");
    if (lvgl_port_lock(-1)) {
      ui_show_offline();
      g_runtime.dashboardHealthy = false;
      g_runtime.workoutsHealthy = false;
      lvgl_port_unlock();
    }
  } else {
    Serial.println("[BOOT] wifi connected; fetching API");
    refresh_from_api();
    refresh_workouts_once();
  }

  const uint32_t bootDoneMs = millis();
  g_runtime.lastDashboardFetchMs = bootDoneMs;
  Serial.println("[BOOT] setup done");
}

void loop() {
  if (!g_displayReady) {
    delay(1000);
    return;
  }

  if (g_displayActivityPending) {
    g_displayActivityPending = false;
    mark_activity_and_wake();
  }

  if (presence_detected()) {
    mark_activity_and_wake();
  }

  const uint32_t now = millis();

  if (ui_should_keep_display_awake()) {
    mark_activity_and_wake();
  } else {
    display_power_handle_inactivity(g_runtime.displayPower, now, BACKLIGHT_OFF_TIMEOUT_MS);
  }

  if ((now - g_runtime.lastDashboardFetchMs) >= DASHBOARD_API_REFRESH_MS) {
    g_runtime.lastDashboardFetchMs = now;
    refresh_from_api();
  }

  refresh_workouts_if_idle(now);

  delay(LVGL_HANDLER_INTERVAL_MS);
}
