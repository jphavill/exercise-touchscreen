# Exercise Touchscreen (ESP32-S3 + LVGL)
ESP32-S3 Arduino project for an 800x480 touchscreen dashboard using LVGL.
The device fetches pull-up stats and workout routines from HTTP APIs, then displays dashboard, workout selection, workout execution, and offline UI states.
Display/backlight activity is driven by a presence sensor, touch events, and active workouts.
## Project layout
- `exercise-touchscreen.ino`
  - Arduino sketch marker; real implementation is in `.cpp/.h` files.
- `main.cpp`
  - Application orchestrator:
    - initializes display/touch/LVGL, power, presence
    - initializes UI and callback wiring
    - connects Wi-Fi and fetches initial dashboard/workout API data
    - runs loop for touch/presence activity, active-workout wake behavior, inactivity sleep, and periodic API refreshes
- `ui.cpp` / `ui.h`
  - UI orchestration, screen navigation, gesture handling, public dashboard setters, and workout list updates.
  - Three LVGL screens:
    - Dashboard (metrics + heatmap)
    - Workout selection (API-backed routine cards or empty state)
    - Workout execution (intro countdown, timed steps, rep steps, completion state)
  - Offline state is shown as a modal overlay instead of replacing the current screen.
  - Refresh button event callback registration.
- `dashboard_screen.cpp` / `dashboard_screen.h`
  - Dashboard screen construction and metric/heatmap setters.
- `workout_selection_screen.cpp` / `workout_selection_screen.h`
  - Workout card grid, empty state, and selection callback wiring.
- `workout_screen.cpp` / `workout_screen.h`
  - Step-based workout execution screen with intro countdown, timed exercise/rest steps, manual rep-step completion, and finished state.
- `workout_cache.cpp` / `workout_cache.h`
  - In-memory workout cache and refresh throttling around the workouts API.
- `offline_modal.cpp` / `offline_modal.h`
  - Offline overlay modal with refresh action.
- `ui_style.cpp` / `ui_style.h`
  - Shared UI fonts, spacing constants, tap/gesture helpers, and heatmap color/opacity helpers.
- `wifi_api.cpp` / `wifi_api.h`
  - Wi-Fi connection helper.
  - HTTP GET helpers for dashboard and workouts endpoints.
  - JSON payload parsing into `PullupDashboardData` and `WorkoutCache`.
- `models.h`
  - Data model:
    - `DayEntry` (`count`, `heatLevel`)
    - `PullupDashboardData` (`yearTotal`, `dailyGoal`, `days[30]`, `valid`)
    - `WorkoutRoutine`, `WorkoutStep`, `WorkoutStepKind`, and `WorkoutCache`
- `presence.cpp` / `presence.h`
  - Presence sensor GPIO setup and debounce logic.
  - Returns debounced HIGH/LOW presence state.
- `power.cpp` / `power.h`
  - Backlight power control.
  - LEDC PWM setup with ESP32 Arduino version compatibility.
  - Digital GPIO fallback if PWM attach fails.
  - Caches backlight state to avoid redundant writes.
- `display_power_controller.cpp` / `display_power_controller.h`
  - Tracks last display activity and sleep state.
  - Turns the backlight on for activity and off after the configured inactivity timeout.
- `lvgl_v8_port.cpp` / `lvgl_v8_port.h`
  - LVGL + panel/touch integration layer.
  - Initializes LVGL display/input drivers and tick timer.
  - Creates dedicated LVGL task.
  - Reports first touch press activity through `lvgl_port_set_touch_activity_callback()`.
  - Exposes LVGL recursive mutex lock/unlock API for thread-safe UI updates.
  - Uses direct-mode tearing avoidance by default with board frame buffers.
- `config.h`
  - Runtime constants:
    - Wi-Fi credentials mapping from `secrets.h`
    - dashboard/workouts API URLs, refresh intervals, and HTTP timeout
    - backlight pin/PWM constants
    - presence pin
    - inactivity timeout and loop cadence
- `lv_conf.h`
  - LVGL compile-time config:
    - color depth, memory alloc strategy, logging
    - enabled layout engines (Flex/Grid)
    - font set and default theme options
- `secrets.h.example`
  - Template for local Wi-Fi credentials (`secrets.h` not committed).
- `esp_panel_*_conf.h`, `esp_utils_conf.h`
  - Espressif panel/utils library compile-time configuration.
  - Board selection and driver trimming for the active RGB bus, ST7262 LCD, GT911 touch, I2C bus, and LEDC PWM backlight setup.
## Runtime flow
1. Boot
   - Serial starts.
   - Display/touch/panel/LVGL are initialized (`init_display_and_touch`).
   - Backlight and presence subsystems initialize.
2. UI bring-up
   - `ui_init()` builds LVGL objects.
   - Main registers:
     - refresh callback (manual fetch)
     - tap wake callback (activity signal)
3. Network + first data
    - Connect Wi-Fi via `wifi_connect()`.
    - Fetch dashboard data via `fetch_dashboard_data()`.
    - Fetch workout routines via `workout_cache_refresh()`.
    - Show dashboard once dashboard data is available; show the offline modal while dashboard data or cached workouts are unavailable/unhealthy.
4. Main loop
    - Pending touch activity and debounced presence polling update the display activity timestamp and wake the backlight.
    - Active unfinished workouts keep the display awake.
    - Inactivity timeout turns backlight off and marks display sleeping when no active workout needs the display.
    - Dashboard refresh runs every `DASHBOARD_API_REFRESH_MS`.
    - Workout cache refresh runs every `WORKOUT_CACHE_REFRESH_MS`, except while the workout page is active or a workout timer is running.
## UI navigation
- `ui_init()` creates one LVGL screen each for dashboard, workout selection, and workout execution, then loads the dashboard screen.
- Swipe left from the dashboard to open workout selection.
- Swipe right from workout selection to return to the dashboard.
- Selecting a workout card opens the workout execution screen with a slide-left animation and starts a 3-second intro countdown.
- The workout execution back button returns to workout selection with a slide-right animation.
- Timed exercise and rest steps advance automatically when their countdown reaches zero.
- Rep exercise steps show a Done button and advance when tapped.
- API failures call `ui_show_offline()`, which displays a top-layer modal with a refresh button.
- The offline modal is hidden only when dashboard data is healthy and workouts are healthy or available from cache.
- If data has already loaded, a later API failure keeps the current screen visible under the offline modal; the next healthy refresh hides the modal.
- If no dashboard data has loaded yet, the first successful fetch loads the dashboard screen.
## Concurrency and rendering model
- LVGL runs in its own task from `lvgl_v8_port.cpp`.
- Any UI mutation from app code is wrapped by:
  - `lvgl_port_lock(-1)`
  - `lvgl_port_unlock()`
- This protects LVGL object updates across tasks.
## Data contract assumptions
- Dashboard API payload must include:
  - `year_total` (uint32)
  - `daily_goal` (uint16)
  - `last_30_days` array of exactly 30 items
- Each day requires:
  - `count` (uint16), `heat_level` (uint8)
- UI currently treats `days[29]` as “today”.
- Workouts API payload may be either a top-level array or an object containing `workouts`.
- Up to `kMaxWorkouts` routines are accepted.
- Each routine requires `id`, `name`, `duration_min`, and a non-empty `steps` array up to `kMaxWorkoutSteps`.
- Each step requires `kind` and `title`.
- Step kinds are `timed_exercise`, `rest`, and `rep_exercise`.
- Timed exercise/rest steps require `duration_sec`; rep exercise steps require `reps` and `rep_unit`.
