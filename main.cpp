#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "models.h"
#include "power.h"
#include "presence.h"
#include "ui.h"
#include "wifi_api.h"

static uint32_t g_lastApiFetchMs = 0;
static uint32_t g_lastInteractionMs = 0;
static uint32_t g_lastLvglTickMs = 0;

static void refresh_from_api();

static void init_display_and_touch() {
  lv_init();

  // Integrate your specific 7-inch TFT panel and touch driver here.
  // Required steps:
  // 1) Initialize display hardware
  // 2) Register LVGL display draw buffer + flush callback
  // 3) Initialize touch controller
  // 4) Register LVGL input device read callback for touch
}

static void mark_activity_and_wake() {
  g_lastInteractionMs = millis();
  backlight_on();
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

  if (api_fetch_data(data) && data.valid) {
    ui_set_today(data.days[29].count);
    ui_set_goal(data.dailyGoal);
    ui_set_year(data.yearTotal);
    ui_set_heatmap(data.days);
    ui_show_dashboard();
  } else {
    ui_show_offline();
  }
}

void setup() {
  Serial.begin(115200);

  init_display_and_touch();
  power_init();
  presence_init();

  ui_init();
  ui_set_refresh_callback(on_refresh_button);
  ui_set_tap_wake_callback(on_tap_wake);

  mark_activity_and_wake();

  if (!wifi_connect()) {
    ui_show_offline();
  } else {
    refresh_from_api();
  }

  g_lastApiFetchMs = millis();
  g_lastLvglTickMs = millis();
}

void loop() {
  const uint32_t now = millis();

  const uint32_t tickDelta = now - g_lastLvglTickMs;
  if (tickDelta > 0) {
    lv_tick_inc(tickDelta);
    g_lastLvglTickMs = now;
  }

  lv_timer_handler();

  if (presence_detected()) {
    mark_activity_and_wake();
  }

  const uint32_t inactiveMs = now - g_lastInteractionMs;
  if (inactiveMs >= BACKLIGHT_OFF_TIMEOUT_MS) {
    backlight_off();
  } else if (inactiveMs >= BACKLIGHT_DIM_TIMEOUT_MS) {
    backlight_dim();
  }

  if ((now - g_lastApiFetchMs) >= API_REFRESH_MS) {
    g_lastApiFetchMs = now;
    refresh_from_api();
  }

  delay(LVGL_HANDLER_INTERVAL_MS);
}
