#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

#include <esp_display_panel.hpp>
#include "lvgl_port.h"

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
static PullupDashboardData g_lastShownData = {};
static bool g_hasLastShownData = false;
static constexpr uint32_t RGB_PCLK_HZ_SAFE = 8 * 1000 * 1000;
static constexpr uint32_t RGB_BOUNCE_LINES = 30;
static const char* TAG = "app";

static void refresh_from_api();

static uint32_t uptime_ms() {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

static bool day_entry_equal(const DayEntry& a, const DayEntry& b) {
  return (a.count == b.count) && (a.heatLevel == b.heatLevel) &&
         (strncmp(a.date, b.date, sizeof(a.date)) == 0);
}

static bool dashboard_data_equal(const PullupDashboardData& a,
                                 const PullupDashboardData& b) {
  if (a.dailyGoal != b.dailyGoal || a.yearTotal != b.yearTotal || a.valid != b.valid) {
    return false;
  }

  for (uint8_t i = 0; i < 30; i++) {
    if (!day_entry_equal(a.days[i], b.days[i])) {
      return false;
    }
  }

  return true;
}

static bool init_display_and_touch() {
  g_board = new Board();
  if (!g_board) {
    ESP_LOGE(TAG, "Failed to allocate Board instance");
    return false;
  }

  if (!g_board->init()) {
    ESP_LOGE(TAG, "Board initialization failed");
    return false;
  }

  auto lcd = g_board->getLCD();
  if (!lcd) {
    ESP_LOGE(TAG, "Board initialized without LCD handle");
    return false;
  }

  const int frameWidth = lcd->getFrameWidth();
  const int frameHeight = lcd->getFrameHeight();
  if (frameWidth <= 0 || frameWidth > 65535 || frameHeight <= 0 || frameHeight > 65535) {
    ESP_LOGE(TAG, "Invalid frame size: %dx%d", frameWidth, frameHeight);
    return false;
  }

  auto lcd_bus = lcd->getBus();
  if (lcd_bus && lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
    auto rgb_bus = static_cast<esp_panel::drivers::BusRGB*>(lcd_bus);
    rgb_bus->configRGB_FreqHz(RGB_PCLK_HZ_SAFE);
    rgb_bus->configRGB_BounceBufferSize(lcd->getFrameWidth() * RGB_BOUNCE_LINES);
  }

  const uint8_t frameBufferCount = lvgl_port_get_required_frame_buffer_count();
  lcd->configFrameBufferNumber(frameBufferCount);
  ESP_LOGI(TAG, "Configured RGB frame buffers: %u", static_cast<unsigned>(frameBufferCount));

  if (!g_board->begin()) {
    return false;
  }

  auto touch = g_board->getTouch();
  auto basicSpec = lcd->getBasicAttributes().basic_bus_spec;
  void* touchHandle = touch ? static_cast<void*>(touch->getPanelHandle()) : nullptr;

  if (!lvgl_port_init(static_cast<void*>(lcd->getRefreshPanelHandle()), touchHandle,
                      static_cast<uint16_t>(frameWidth), static_cast<uint16_t>(frameHeight),
                      basicSpec.x_coord_align, basicSpec.y_coord_align)) {
    ESP_LOGE(TAG, "LVGL adapter initialization failed");
    return false;
  }

  return true;
}

static void mark_activity_and_wake() {
  g_lastInteractionMs = uptime_ms();
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
    const bool changed = !g_hasLastShownData || !dashboard_data_equal(g_lastShownData, data);
    if (changed) {
      ui_set_today(data.days[29].count);
      ui_set_goal(data.dailyGoal);
      ui_set_year(data.yearTotal);
      ui_set_heatmap(data.days);
      g_lastShownData = data;
      g_hasLastShownData = true;
    }
    ui_show_dashboard();
  } else {
    ui_show_offline();
  }

  lvgl_port_unlock();
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Starting application");
#if CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED
  ESP_LOGI(TAG, "ESP Panel board mode: supported board (menuconfig)");
#elif CONFIG_ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
  ESP_LOGI(TAG, "ESP Panel board mode: custom board (menuconfig)");
#else
  ESP_LOGW(TAG, "ESP Panel board mode: none selected");
#endif

  power_init();
  presence_init();
  g_displayReady = init_display_and_touch();

  if (!g_displayReady) {
    ESP_LOGE(TAG, "Display initialization failed");
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  if (lvgl_port_lock(-1)) {
    ui_init();
    lvgl_port_unlock();
  }
  ui_set_refresh_callback(on_refresh_button);
  ui_set_tap_wake_callback(on_tap_wake);

  mark_activity_and_wake();

  if (!wifi_connect()) {
    ESP_LOGW(TAG, "Wi-Fi not connected; showing offline state");
    if (lvgl_port_lock(-1)) {
      ui_show_offline();
      lvgl_port_unlock();
    }
  } else {
    refresh_from_api();
  }

  g_lastApiFetchMs = uptime_ms();

  while (true) {
    if (!g_displayReady) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    const uint32_t now = uptime_ms();

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

    vTaskDelay(pdMS_TO_TICKS(LVGL_HANDLER_INTERVAL_MS));
  }
}
