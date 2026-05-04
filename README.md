# Exercise Touchscreen (ESP32-S3 + LVGL)
ESP32-S3 Arduino project for an 800x480 touchscreen dashboard using LVGL.
The device fetches pull-up stats from an HTTP API and displays either a dashboard or offline view.
Display/backlight activity is driven by a presence sensor and touch events.
## Project layout
- `exercise-touchscreen.ino`
  - Arduino sketch marker; real implementation is in `.cpp/.h` files.
- `main.cpp`
  - Application orchestrator:
    - initializes display/touch/LVGL, power, presence
    - initializes UI and callback wiring
    - connects Wi-Fi and fetches initial API data
    - runs loop for touch/presence activity, inactivity sleep, and periodic API refresh
- `ui.cpp` / `ui.h`
  - LVGL object tree and view-state handling.
  - Two top-level states:
    - Dashboard (metrics + heatmap)
    - Offline (message + refresh button)
  - Public setters for metrics and heatmap.
  - Refresh button event callback registration.
- `wifi_api.cpp` / `wifi_api.h`
  - Wi-Fi connection helper.
  - HTTP GET to API endpoint with timezone header.
  - JSON payload parsing into `PullupDashboardData`.
- `models.h`
  - Data model:
    - `DayEntry` (`count`, `heatLevel`)
    - `PullupDashboardData` (`yearTotal`, `dailyGoal`, `days[30]`, `valid`)
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
    - API URL and refresh interval
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
   - Fetch API data via `api_fetch_data()`.
   - Show dashboard on success, offline screen on failure.
4. Main loop
   - Pending touch activity and debounced presence polling update the display activity timestamp and wake the backlight.
   - Inactivity timeout turns backlight off and marks display sleeping.
   - Periodic API refresh runs every `API_REFRESH_MS`.
## Concurrency and rendering model
- LVGL runs in its own task from `lvgl_v8_port.cpp`.
- Any UI mutation from app code is wrapped by:
  - `lvgl_port_lock(-1)`
  - `lvgl_port_unlock()`
- This protects LVGL object updates across tasks.
## Data contract assumptions
- API payload must include:
  - `year_total` (uint32)
  - `daily_goal` (uint16)
  - `last_30_days` array of exactly 30 items
- Each day requires:
  - `count` (uint16), `heat_level` (uint8)
- UI currently treats `days[29]` as “today”.
