#pragma once

#include "models.h"

typedef void (*ui_action_callback_t)();

void ui_init();
void ui_show_dashboard();
void ui_show_offline();
void ui_hide_offline();
void ui_set_today(uint16_t todayCount);
void ui_set_goal(uint16_t dailyGoal);
void ui_set_year(uint32_t yearTotal);
void ui_set_heatmap(const DayEntry days[30]);
void ui_set_all(const PullupDashboardData& data);

void ui_set_refresh_callback(ui_action_callback_t callback);
