#pragma once

#include "models.h"
#include "ui.h"

#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#include <lvgl.h>

struct DashboardScreen {
  lv_obj_t* root = nullptr;
  lv_obj_t* todayLabel = nullptr;
  lv_obj_t* goalLabel = nullptr;
  lv_obj_t* yearLabel = nullptr;
  lv_obj_t* heatCells[30] = {nullptr};
};

DashboardScreen dashboard_screen_create(lv_event_cb_t gestureCallback,
                                        ui_action_callback_t refreshCallback);

void dashboard_screen_set_today(DashboardScreen& screen, uint16_t todayCount);
void dashboard_screen_set_goal(DashboardScreen& screen, uint16_t dailyGoal);
void dashboard_screen_set_year(DashboardScreen& screen, uint32_t yearTotal);
void dashboard_screen_set_heatmap(DashboardScreen& screen, const DayEntry days[30]);
void dashboard_screen_set_all(DashboardScreen& screen, const PullupDashboardData& data);
