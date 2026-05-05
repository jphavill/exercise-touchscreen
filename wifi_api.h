#pragma once

#include "models.h"

bool wifi_connect();
bool fetch_dashboard_data(PullupDashboardData& outData);
bool fetch_workouts(WorkoutCache& outCache);
