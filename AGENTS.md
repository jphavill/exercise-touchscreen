# Exercise Touchscreen Agent Notes

ESP32-S3 Arduino project for an 800x480 LVGL touchscreen. It fetches pull-up dashboard data and workout routines from HTTP APIs, displays dashboard/workout/offline UI states, and manages display activity from touch, a presence sensor, and active workouts.

## Key Files

- `exercise-touchscreen.ino`: sketch marker; implementation lives in `.cpp`/`.h` files.
- `main.cpp`: app orchestration, display/LVGL init, Wi-Fi/API fetches, dashboard/workout refresh loops, presence/touch activity, active-workout wake behavior, sleep handling.
- `ui.cpp` / `ui.h`: UI orchestration, LVGL screen navigation, dashboard/workout gesture routing, public metric setters, workout list updates, refresh callback wiring.
- `dashboard_screen.cpp` / `dashboard_screen.h`: dashboard screen object tree, metric labels, 30-day heatmap, dashboard refresh button.
- `workout_selection_screen.cpp` / `workout_selection_screen.h`: workout card grid, empty state, selected-routine callback.
- `workout_screen.cpp` / `workout_screen.h`: step-based workout execution screen with intro countdown, timed steps, rep-step Done button, completion state.
- `workout_cache.cpp` / `workout_cache.h`: in-memory workout cache and refresh throttling around the workouts API.
- `offline_modal.cpp` / `offline_modal.h`: top-layer offline modal with refresh action; does not replace the active screen.
- `ui_style.cpp` / `ui_style.h`: shared fonts, spacing constants, tap/gesture helpers, heatmap color/opacity helpers.
- `wifi_api.cpp` / `wifi_api.h`: Wi-Fi connect and dashboard/workouts HTTP API fetch/parsing into `PullupDashboardData` and `WorkoutCache`.
- `models.h`: `DayEntry`, `PullupDashboardData`, `WorkoutRoutine`, `WorkoutStep`, `WorkoutStepKind`, and `WorkoutCache` data contracts.
- `lvgl_v8_port.cpp` / `lvgl_v8_port.h`: LVGL display/input integration and recursive mutex helpers.
- `display_power_controller.cpp`, `power.cpp`, `presence.cpp`: display sleep/backlight/presence support.
- `config.h`: runtime constants, dashboard/workouts API URLs, Wi-Fi secret mapping, GPIOs, refresh and timeout values.
- `lv_conf.h`, `esp_panel_*_conf.h`, `esp_utils_conf.h`: LVGL and Espressif panel compile-time configuration.
- `secrets.h.example`: local Wi-Fi credential template; `secrets.h` is not committed.

## Runtime Notes

- LVGL runs in its own task; app-side UI mutations should be wrapped with `lvgl_port_lock(-1)` and `lvgl_port_unlock()`.
- `ui_init()` creates separate LVGL screens for dashboard, workout selection, and workout execution, then loads the dashboard screen.
- Dashboard swipe-left opens workout selection; workout selection swipe-right returns to dashboard.
- Workout card selection opens workout execution, starts a 3-second intro countdown, and then advances through timed/rest steps automatically or rep steps via Done.
- The workout back button stops the active workout timer and returns to workout selection.
- Active unfinished workouts keep the display awake; idle workout cache refresh is skipped while the workout page is active or a workout timer is running.
- Dashboard API success updates dashboard data. The first dashboard success calls `ui_show_dashboard()`.
- Workout API success updates the cache and repopulates workout cards; cached workouts can keep the workouts side healthy after a later fetch failure.
- API failure calls `ui_show_offline()`, which shows an overlay modal and preserves any already-loaded dashboard/workout screen underneath.
- The offline modal is hidden only when dashboard data is healthy and workouts are healthy or available from cache.
- Dashboard data expects `year_total`, `daily_goal`, and exactly 30 `last_30_days` entries with `count` and `heat_level`.
- `days[29]` is treated as today.
- Workouts data accepts either a top-level array or an object containing `workouts`, capped at `kMaxWorkouts` routines.
- Workout routines require `id`, `name`, `duration_min`, and non-empty `steps` capped at `kMaxWorkoutSteps`.
- Workout step kinds are `timed_exercise`, `rest`, and `rep_exercise`; timed/rest steps require `duration_sec`, rep steps require `reps` and `rep_unit`.

## Compile

Use Arduino CLI with the ESP32-S3 board and the Huge APP partition. The default partition is too small for this sketch.

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app .
```

In Arduino IDE, select `ESP32S3 Dev Module` and `Huge APP (3MB No OTA/1MB SPIFFS)` for the partition scheme.
