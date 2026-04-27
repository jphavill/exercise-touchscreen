#include "ui.h"

#include <Arduino.h>

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

#if LV_FONT_MONTSERRAT_48
#define UI_FONT_TODAY (&lv_font_montserrat_48)
#else
#define UI_FONT_TODAY LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_26
#define UI_FONT_TITLE (&lv_font_montserrat_26)
#else
#define UI_FONT_TITLE LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_24
#define UI_FONT_OFFLINE_SUB (&lv_font_montserrat_24)
#else
#define UI_FONT_OFFLINE_SUB LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_42
#define UI_FONT_OFFLINE_TITLE (&lv_font_montserrat_42)
#else
#define UI_FONT_OFFLINE_TITLE LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_22
#define UI_FONT_META (&lv_font_montserrat_22)
#else
#define UI_FONT_META LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_20
#define UI_FONT_BUTTON (&lv_font_montserrat_20)
#else
#define UI_FONT_BUTTON LV_FONT_DEFAULT
#endif

static lv_obj_t* g_dashboardContainer = nullptr;
static lv_obj_t* g_offlineContainer = nullptr;
static lv_obj_t* g_todayLabel = nullptr;
static lv_obj_t* g_goalLabel = nullptr;
static lv_obj_t* g_yearLabel = nullptr;
static lv_obj_t* g_refreshBtn = nullptr;
static lv_obj_t* g_heatCells[30] = {nullptr};

static ui_action_callback_t g_refreshCallback = nullptr;
static ui_action_callback_t g_tapWakeCallback = nullptr;

static constexpr lv_coord_t kHeatCellSize = 46;
static constexpr lv_coord_t kHeatGridGap = 10;
static constexpr lv_coord_t kHeatWrapPad = 8;
static constexpr uint8_t kHeatColCount = 10;
static constexpr uint8_t kHeatRowCount = 3;

static lv_coord_t kHeatCols[] = {kHeatCellSize, kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 kHeatCellSize, kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 kHeatCellSize, kHeatCellSize, LV_GRID_TEMPLATE_LAST};
static lv_coord_t kHeatRows[] = {kHeatCellSize, kHeatCellSize, kHeatCellSize, LV_GRID_TEMPLATE_LAST};

static lv_color_t heat_color(uint8_t level) {
  if (level == 0) {
    return lv_color_hex(0xFFFFFF);
  }
  return lv_color_hex(0x78C7FF);
}

static lv_opa_t heat_opa(uint8_t level) {
  switch (level) {
    case 0:
      return static_cast<lv_opa_t>(15);   // ~6%
    case 1:
      return static_cast<lv_opa_t>(56);   // ~22%
    case 2:
      return static_cast<lv_opa_t>(115);  // ~45%
    case 3:
      return static_cast<lv_opa_t>(173);  // ~68%
    default:
      return static_cast<lv_opa_t>(235);  // ~92%
  }
}

static void on_refresh_btn(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && g_refreshCallback != nullptr) {
    g_refreshCallback();
  }
}

static void on_tap_wake(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_PRESSED && g_tapWakeCallback != nullptr) {
    g_tapWakeCallback();
  }
}

static void register_tap_target(lv_obj_t* obj) {
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(obj, on_tap_wake, LV_EVENT_PRESSED, nullptr);
}

static void apply_base_screen_style(lv_obj_t* obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
}

void ui_init() {
  lv_obj_t* scr = lv_scr_act();
  apply_base_screen_style(scr);

  register_tap_target(scr);

  g_dashboardContainer = lv_obj_create(scr);
  lv_obj_remove_style_all(g_dashboardContainer);
  lv_obj_set_size(g_dashboardContainer, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(g_dashboardContainer, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(g_dashboardContainer, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(g_dashboardContainer, 24, 0);
  lv_obj_set_layout(g_dashboardContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(g_dashboardContainer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_dashboardContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  register_tap_target(g_dashboardContainer);

  lv_obj_t* metricsCol = lv_obj_create(g_dashboardContainer);
  lv_obj_remove_style_all(metricsCol);
  lv_obj_set_size(metricsCol, LV_PCT(44), LV_PCT(100));
  lv_obj_set_layout(metricsCol, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(metricsCol, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(metricsCol, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(metricsCol, 12, 0);
  register_tap_target(metricsCol);

  lv_obj_t* titleLabel = lv_label_create(metricsCol);
  lv_label_set_text(titleLabel, "Pull-ups");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(titleLabel, UI_FONT_TITLE, 0);

  g_todayLabel = lv_label_create(metricsCol);
  lv_label_set_text(g_todayLabel, "0");
  lv_obj_set_style_text_color(g_todayLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(g_todayLabel, UI_FONT_TODAY, 0);

  g_goalLabel = lv_label_create(metricsCol);
  lv_label_set_text(g_goalLabel, "Goal 0");
  lv_obj_set_style_text_color(g_goalLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(g_goalLabel, UI_FONT_META, 0);

  g_yearLabel = lv_label_create(metricsCol);
  lv_label_set_text(g_yearLabel, "Year 0");
  lv_obj_set_style_text_color(g_yearLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(g_yearLabel, UI_FONT_META, 0);

  g_refreshBtn = lv_btn_create(scr);
  lv_obj_set_size(g_refreshBtn, 160, 54);
  lv_obj_set_style_bg_color(g_refreshBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(g_refreshBtn, LV_OPA_70, 0);
  lv_obj_set_style_radius(g_refreshBtn, 12, 0);
  lv_obj_set_style_border_width(g_refreshBtn, 0, 0);
  lv_obj_add_event_cb(g_refreshBtn, on_refresh_btn, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(g_refreshBtn, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(g_refreshBtn, LV_ALIGN_TOP_RIGHT, -10, 10);
  register_tap_target(g_refreshBtn);

  lv_obj_t* refreshLabel = lv_label_create(g_refreshBtn);
  lv_label_set_text(refreshLabel, "Refresh");
  lv_obj_set_style_text_color(refreshLabel, lv_color_hex(0x041017), 0);
  lv_obj_set_style_text_font(refreshLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(refreshLabel);

  lv_obj_t* heatmapWrap = lv_obj_create(g_dashboardContainer);
  lv_obj_remove_style_all(heatmapWrap);
  lv_obj_set_size(
      heatmapWrap,
      static_cast<lv_coord_t>(kHeatColCount * kHeatCellSize + (kHeatColCount - 1) * kHeatGridGap +
                              2 * kHeatWrapPad),
      static_cast<lv_coord_t>(kHeatRowCount * kHeatCellSize + (kHeatRowCount - 1) * kHeatGridGap +
                              2 * kHeatWrapPad));
  lv_obj_set_style_pad_all(heatmapWrap, kHeatWrapPad, 0);
  lv_obj_set_layout(heatmapWrap, LV_LAYOUT_GRID);
  lv_obj_set_style_grid_column_dsc_array(heatmapWrap, kHeatCols, 0);
  lv_obj_set_style_grid_row_dsc_array(heatmapWrap, kHeatRows, 0);
  lv_obj_set_style_pad_row(heatmapWrap, kHeatGridGap, 0);
  lv_obj_set_style_pad_column(heatmapWrap, kHeatGridGap, 0);
  register_tap_target(heatmapWrap);

  for (uint8_t i = 0; i < 30; i++) {
    lv_obj_t* cell = lv_obj_create(heatmapWrap);
    lv_obj_remove_style_all(cell);
    lv_obj_set_size(cell, kHeatCellSize, kHeatCellSize);
    lv_obj_set_style_radius(cell, 6, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_bg_color(cell, lv_color_hex(0x78C7FF), 0);
    lv_obj_set_style_bg_opa(cell, heat_opa(0), 0);

    const uint8_t col = i % 10;
    const uint8_t row = i / 10;
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    register_tap_target(cell);
    g_heatCells[i] = cell;
  }

  g_offlineContainer = lv_obj_create(scr);
  lv_obj_remove_style_all(g_offlineContainer);
  lv_obj_set_size(g_offlineContainer, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(g_offlineContainer, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(g_offlineContainer, LV_OPA_COVER, 0);
  lv_obj_set_layout(g_offlineContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(g_offlineContainer, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_offlineContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(g_offlineContainer, 8, 0);
  register_tap_target(g_offlineContainer);

  lv_obj_t* offlineTitle = lv_label_create(g_offlineContainer);
  lv_label_set_text(offlineTitle, "Offline");
  lv_obj_set_style_text_color(offlineTitle, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(offlineTitle, UI_FONT_OFFLINE_TITLE, 0);

  lv_obj_t* offlineSub = lv_label_create(g_offlineContainer);
  lv_label_set_text(offlineSub, "Unable to reach API");
  lv_obj_set_style_text_color(offlineSub, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(offlineSub, UI_FONT_OFFLINE_SUB, 0);

  ui_show_offline();
}

void ui_show_dashboard() {
  if (g_dashboardContainer == nullptr || g_offlineContainer == nullptr || g_refreshBtn == nullptr) {
    return;
  }
  lv_obj_clear_flag(g_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(g_refreshBtn, LV_OBJ_FLAG_HIDDEN);
}

void ui_show_offline() {
  if (g_dashboardContainer == nullptr || g_offlineContainer == nullptr || g_refreshBtn == nullptr) {
    return;
  }
  lv_obj_add_flag(g_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(g_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_refreshBtn, LV_OBJ_FLAG_HIDDEN);
}

void ui_set_today(uint16_t todayCount) {
  if (g_todayLabel == nullptr) {
    return;
  }

  char buf[12];
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(todayCount));
  lv_label_set_text(g_todayLabel, buf);
}

void ui_set_goal(uint16_t dailyGoal) {
  if (g_goalLabel == nullptr) {
    return;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), "Goal %u", static_cast<unsigned>(dailyGoal));
  lv_label_set_text(g_goalLabel, buf);
}

void ui_set_year(uint32_t yearTotal) {
  if (g_yearLabel == nullptr) {
    return;
  }

  char buf[28];
  snprintf(buf, sizeof(buf), "Year %lu", static_cast<unsigned long>(yearTotal));
  lv_label_set_text(g_yearLabel, buf);
}

void ui_set_heatmap(const DayEntry days[30]) {
  for (uint8_t i = 0; i < 30; i++) {
    if (g_heatCells[i] == nullptr) {
      continue;
    }

    const uint8_t level = days[i].heatLevel;
    lv_obj_set_style_bg_color(g_heatCells[i], heat_color(level), 0);
    lv_obj_set_style_bg_opa(g_heatCells[i], heat_opa(level), 0);
  }
}

void ui_set_all(const PullupDashboardData& data) {
  if (!data.valid) {
    return;
  }

  ui_set_today(data.days[29].count);
  ui_set_goal(data.dailyGoal);
  ui_set_year(data.yearTotal);
  ui_set_heatmap(data.days);
}

void ui_set_refresh_callback(ui_action_callback_t callback) {
  g_refreshCallback = callback;
}

void ui_set_tap_wake_callback(ui_action_callback_t callback) {
  g_tapWakeCallback = callback;
}
