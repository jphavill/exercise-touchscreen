#include "ui.h"

#include <cstdio>
#include <cstring>

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
static lv_obj_t* g_heatCells[30] = {nullptr};
static uint8_t g_lastHeatLevels[30] = {0};
static bool g_heatmapInitialized = false;

static ui_action_callback_t g_refreshCallback = nullptr;
static ui_action_callback_t g_tapWakeCallback = nullptr;

static constexpr lv_coord_t kHeatCellSize = 58;
static constexpr lv_coord_t kHeatGap = 8;
static constexpr lv_coord_t kHeatPad = 8;
static lv_coord_t kHeatCols[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
                                 LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
static lv_coord_t kHeatRows[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
                                 LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
                                 LV_GRID_TEMPLATE_LAST};

static lv_color_t heat_color(uint8_t level) {
  switch (level) {
    case 0:
      return lv_color_hex(0x0F0F0F);
    case 1:
      return lv_color_hex(0x1A2C38);
    case 2:
      return lv_color_hex(0x365A73);
    case 3:
      return lv_color_hex(0x5187AD);
    default:
      return lv_color_hex(0x6EB8EA);
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
  lv_obj_set_style_pad_all(g_dashboardContainer, 18, 0);
  lv_obj_set_layout(g_dashboardContainer, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(g_dashboardContainer, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(g_dashboardContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(g_dashboardContainer, 10, 0);
  register_tap_target(g_dashboardContainer);

  lv_obj_t* topRow = lv_obj_create(g_dashboardContainer);
  lv_obj_remove_style_all(topRow);
  lv_obj_set_size(topRow, LV_PCT(100), LV_PCT(24));
  lv_obj_set_layout(topRow, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(topRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(topRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  register_tap_target(topRow);

  lv_obj_t* pullupsGroup = lv_obj_create(topRow);
  lv_obj_remove_style_all(pullupsGroup);
  lv_obj_set_layout(pullupsGroup, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(pullupsGroup, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(pullupsGroup, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(pullupsGroup, 2, 0);
  register_tap_target(pullupsGroup);

  lv_obj_t* titleLabel = lv_label_create(pullupsGroup);
  lv_label_set_text(titleLabel, "Pull-ups");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(titleLabel, UI_FONT_TITLE, 0);

  g_todayLabel = lv_label_create(pullupsGroup);
  lv_label_set_text(g_todayLabel, "0");
  lv_obj_set_style_text_color(g_todayLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(g_todayLabel, UI_FONT_TODAY, 0);

  g_goalLabel = lv_label_create(topRow);
  lv_label_set_text(g_goalLabel, "Goal 0");
  lv_obj_set_style_text_color(g_goalLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(g_goalLabel, UI_FONT_META, 0);

  g_yearLabel = lv_label_create(topRow);
  lv_label_set_text(g_yearLabel, "Year 0");
  lv_obj_set_style_text_color(g_yearLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(g_yearLabel, UI_FONT_META, 0);

  lv_obj_t* refreshBtn = lv_btn_create(topRow);
  lv_obj_set_size(refreshBtn, 64, 64);
  lv_obj_set_style_bg_color(refreshBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(refreshBtn, LV_OPA_70, 0);
  lv_obj_set_style_radius(refreshBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(refreshBtn, 0, 0);
  lv_obj_add_event_cb(refreshBtn, on_refresh_btn, LV_EVENT_CLICKED, nullptr);
  register_tap_target(refreshBtn);

  lv_obj_t* refreshLabel = lv_label_create(refreshBtn);
  lv_label_set_text(refreshLabel, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_color(refreshLabel, lv_color_hex(0x041017), 0);
  lv_obj_set_style_text_font(refreshLabel, UI_FONT_TITLE, 0);
  lv_obj_center(refreshLabel);

  lv_obj_t* heatmapWrap = lv_obj_create(g_dashboardContainer);
  lv_obj_remove_style_all(heatmapWrap);
  lv_obj_set_size(heatmapWrap,
                  (kHeatCellSize * 5) + (kHeatGap * 4) + (kHeatPad * 2),
                  (kHeatCellSize * 6) + (kHeatGap * 5) + (kHeatPad * 2));
  lv_obj_set_style_pad_all(heatmapWrap, kHeatPad, 0);
  lv_obj_set_layout(heatmapWrap, LV_LAYOUT_GRID);
  lv_obj_set_style_grid_column_dsc_array(heatmapWrap, kHeatCols, 0);
  lv_obj_set_style_grid_row_dsc_array(heatmapWrap, kHeatRows, 0);
  lv_obj_set_style_pad_row(heatmapWrap, kHeatGap, 0);
  lv_obj_set_style_pad_column(heatmapWrap, kHeatGap, 0);
  register_tap_target(heatmapWrap);

  for (uint8_t i = 0; i < 30; i++) {
    lv_obj_t* cell = lv_obj_create(heatmapWrap);
    lv_obj_remove_style_all(cell);
    lv_obj_set_size(cell, kHeatCellSize, kHeatCellSize);
    lv_obj_set_style_radius(cell, 6, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_bg_color(cell, heat_color(0), 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);

    const uint8_t col = i % 5;
    const uint8_t row = i / 5;
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, col, 1, LV_GRID_ALIGN_CENTER, row, 1);
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
  if (g_dashboardContainer == nullptr || g_offlineContainer == nullptr) {
    return;
  }
  const bool dashboardHidden = lv_obj_has_flag(g_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  const bool offlineHidden = lv_obj_has_flag(g_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  if (!dashboardHidden && offlineHidden) {
    return;
  }
  lv_obj_clear_flag(g_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_offlineContainer, LV_OBJ_FLAG_HIDDEN);
}

void ui_show_offline() {
  if (g_dashboardContainer == nullptr || g_offlineContainer == nullptr) {
    return;
  }
  const bool dashboardHidden = lv_obj_has_flag(g_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  const bool offlineHidden = lv_obj_has_flag(g_offlineContainer, LV_OBJ_FLAG_HIDDEN);
  if (dashboardHidden && !offlineHidden) {
    return;
  }
  lv_obj_add_flag(g_dashboardContainer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(g_offlineContainer, LV_OBJ_FLAG_HIDDEN);
}

void ui_set_today(uint16_t todayCount) {
  if (g_todayLabel == nullptr) {
    return;
  }

  char buf[12];
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(todayCount));
  const char* current = lv_label_get_text(g_todayLabel);
  if (current == nullptr || strcmp(current, buf) != 0) {
    lv_label_set_text(g_todayLabel, buf);
  }
}

void ui_set_goal(uint16_t dailyGoal) {
  if (g_goalLabel == nullptr) {
    return;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), "Goal %u", static_cast<unsigned>(dailyGoal));
  const char* current = lv_label_get_text(g_goalLabel);
  if (current == nullptr || strcmp(current, buf) != 0) {
    lv_label_set_text(g_goalLabel, buf);
  }
}

void ui_set_year(uint32_t yearTotal) {
  if (g_yearLabel == nullptr) {
    return;
  }

  char buf[28];
  snprintf(buf, sizeof(buf), "Year %lu", static_cast<unsigned long>(yearTotal));
  const char* current = lv_label_get_text(g_yearLabel);
  if (current == nullptr || strcmp(current, buf) != 0) {
    lv_label_set_text(g_yearLabel, buf);
  }
}

void ui_set_heatmap(const DayEntry days[30]) {
  for (uint8_t i = 0; i < 30; i++) {
    if (g_heatCells[i] == nullptr) {
      continue;
    }

    const uint8_t level = days[i].heatLevel;
    if (g_heatmapInitialized && g_lastHeatLevels[i] == level) {
      continue;
    }
    lv_obj_set_style_bg_color(g_heatCells[i], heat_color(level), 0);
    g_lastHeatLevels[i] = level;
  }

  g_heatmapInitialized = true;
}

void ui_set_refresh_callback(ui_action_callback_t callback) {
  g_refreshCallback = callback;
}

void ui_set_tap_wake_callback(ui_action_callback_t callback) {
  g_tapWakeCallback = callback;
}
