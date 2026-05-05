#include "dashboard_screen.h"

#include "ui_style.h"

#include <Arduino.h>

static constexpr lv_coord_t kHeatCellSize = 46;
static constexpr lv_coord_t kHeatGridGap = 10;
static constexpr lv_coord_t kHeatWrapPad = 8;
static constexpr uint8_t kHeatColCount = 10;
static constexpr uint8_t kHeatRowCount = 3;

static lv_coord_t kHeatCols[] = {kHeatCellSize, kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 kHeatCellSize, kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 kHeatCellSize, kHeatCellSize, LV_GRID_TEMPLATE_LAST};
static lv_coord_t kHeatRows[] = {kHeatCellSize, kHeatCellSize, kHeatCellSize,
                                 LV_GRID_TEMPLATE_LAST};

static ui_action_callback_t s_refreshCallback = nullptr;

static void on_refresh_btn(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  if (s_refreshCallback != nullptr) {
    s_refreshCallback();
  }
}

DashboardScreen dashboard_screen_create(lv_event_cb_t gestureCallback,
                                        ui_action_callback_t refreshCallback) {
  DashboardScreen screen;
  s_refreshCallback = refreshCallback;

  screen.root = lv_obj_create(nullptr);
  ui_apply_base_screen_style(screen.root);
  lv_obj_set_style_pad_all(screen.root, kScreenPad, 0);
  lv_obj_set_layout(screen.root, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(screen.root, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(screen.root, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  ui_register_gesture_screen(screen.root, gestureCallback);

  lv_obj_t* metricsCol = lv_obj_create(screen.root);
  lv_obj_remove_style_all(metricsCol);
  lv_obj_set_size(metricsCol, LV_PCT(44), LV_PCT(100));
  lv_obj_set_layout(metricsCol, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(metricsCol, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(metricsCol, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(metricsCol, 12, 0);
  ui_register_tap_target(metricsCol);

  lv_obj_t* titleLabel = lv_label_create(metricsCol);
  lv_label_set_text(titleLabel, "Pull-ups");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(titleLabel, UI_FONT_TITLE, 0);

  screen.todayLabel = lv_label_create(metricsCol);
  lv_label_set_text(screen.todayLabel, "0");
  lv_obj_set_style_text_color(screen.todayLabel, lv_color_hex(0xF2F7FF), 0);
  lv_obj_set_style_text_font(screen.todayLabel, UI_FONT_TODAY, 0);

  screen.goalLabel = lv_label_create(metricsCol);
  lv_label_set_text(screen.goalLabel, "Goal 0");
  lv_obj_set_style_text_color(screen.goalLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(screen.goalLabel, UI_FONT_META, 0);

  screen.yearLabel = lv_label_create(metricsCol);
  lv_label_set_text(screen.yearLabel, "Year 0");
  lv_obj_set_style_text_color(screen.yearLabel, lv_color_hex(0xB9C8D8), 0);
  lv_obj_set_style_text_font(screen.yearLabel, UI_FONT_META, 0);

  lv_obj_t* refreshBtn = lv_btn_create(screen.root);
  lv_obj_set_size(refreshBtn, 160, 54);
  lv_obj_set_style_bg_color(refreshBtn, lv_color_hex(0x78C7FF), 0);
  lv_obj_set_style_bg_opa(refreshBtn, LV_OPA_70, 0);
  lv_obj_set_style_radius(refreshBtn, 12, 0);
  lv_obj_set_style_border_width(refreshBtn, 0, 0);
  lv_obj_add_event_cb(refreshBtn, on_refresh_btn, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_flag(refreshBtn, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(refreshBtn, LV_ALIGN_TOP_RIGHT, -10, 10);
  ui_register_tap_target(refreshBtn);

  lv_obj_t* refreshLabel = lv_label_create(refreshBtn);
  lv_label_set_text(refreshLabel, "Refresh");
  lv_obj_set_style_text_color(refreshLabel, lv_color_hex(0x041017), 0);
  lv_obj_set_style_text_font(refreshLabel, UI_FONT_BUTTON, 0);
  lv_obj_center(refreshLabel);

  lv_obj_t* heatmapWrap = lv_obj_create(screen.root);
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
  ui_register_tap_target(heatmapWrap);

  for (uint8_t i = 0; i < 30; i++) {
    lv_obj_t* cell = lv_obj_create(heatmapWrap);
    lv_obj_remove_style_all(cell);
    lv_obj_set_size(cell, kHeatCellSize, kHeatCellSize);
    lv_obj_set_style_radius(cell, 6, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_bg_color(cell, lv_color_hex(0x78C7FF), 0);
    lv_obj_set_style_bg_opa(cell, ui_heat_opa(0), 0);

    const uint8_t col = i % 10;
    const uint8_t row = i / 10;
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    ui_register_tap_target(cell);
    screen.heatCells[i] = cell;
  }

  return screen;
}

void dashboard_screen_set_today(DashboardScreen& screen, uint16_t todayCount) {
  if (screen.todayLabel == nullptr) {
    return;
  }

  char buf[12];
  snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(todayCount));
  lv_label_set_text(screen.todayLabel, buf);
}

void dashboard_screen_set_goal(DashboardScreen& screen, uint16_t dailyGoal) {
  if (screen.goalLabel == nullptr) {
    return;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), "Goal %u", static_cast<unsigned>(dailyGoal));
  lv_label_set_text(screen.goalLabel, buf);
}

void dashboard_screen_set_year(DashboardScreen& screen, uint32_t yearTotal) {
  if (screen.yearLabel == nullptr) {
    return;
  }

  char buf[28];
  snprintf(buf, sizeof(buf), "Year %lu", static_cast<unsigned long>(yearTotal));
  lv_label_set_text(screen.yearLabel, buf);
}

void dashboard_screen_set_heatmap(DashboardScreen& screen, const DayEntry days[30]) {
  for (uint8_t i = 0; i < 30; i++) {
    if (screen.heatCells[i] == nullptr) {
      continue;
    }

    const uint8_t level = days[i].heatLevel;
    lv_obj_set_style_bg_color(screen.heatCells[i], ui_heat_color(level), 0);
    lv_obj_set_style_bg_opa(screen.heatCells[i], ui_heat_opa(level), 0);
  }
}

void dashboard_screen_set_all(DashboardScreen& screen, const PullupDashboardData& data) {
  if (!data.valid) {
    return;
  }

  dashboard_screen_set_today(screen, data.days[29].count);
  dashboard_screen_set_goal(screen, data.dailyGoal);
  dashboard_screen_set_year(screen, data.yearTotal);
  dashboard_screen_set_heatmap(screen, data.days);
}
