# Exercise Touchscreen Agent Notes

ESP32-S3 Arduino project for an 800x480 LVGL touchscreen. It fetches pull-up dashboard data from an HTTP API, displays dashboard/workout/offline UI states, and manages display activity from touch plus a presence sensor.

## Key Files

- `exercise-touchscreen.ino`: sketch marker; implementation lives in `.cpp`/`.h` files.
- `main.cpp`: app orchestration, display/LVGL init, Wi-Fi/API fetches, refresh loop, presence/touch activity, sleep handling.
- `ui.cpp` / `ui.h`: LVGL object tree, dashboard/workout/offline navigation, public metric setters, refresh callback wiring.
- `wifi_api.cpp` / `wifi_api.h`: Wi-Fi connect and HTTP API fetch/parsing into `PullupDashboardData`.
- `models.h`: `DayEntry` and `PullupDashboardData` data contracts.
- `lvgl_v8_port.cpp` / `lvgl_v8_port.h`: LVGL display/input integration and recursive mutex helpers.
- `display_power_controller.cpp`, `power.cpp`, `presence.cpp`: display sleep/backlight/presence support.
- `config.h`: runtime constants, API URL, Wi-Fi secret mapping, GPIOs, refresh and timeout values.
- `lv_conf.h`, `esp_panel_*_conf.h`, `esp_utils_conf.h`: LVGL and Espressif panel compile-time configuration.
- `secrets.h.example`: local Wi-Fi credential template; `secrets.h` is not committed.

## Runtime Notes

- LVGL runs in its own task; app-side UI mutations should be wrapped with `lvgl_port_lock(-1)` and `lvgl_port_unlock()`.
- API success calls `ui_show_dashboard()`; API failure calls `ui_show_offline()`.
- Dashboard data expects `year_total`, `daily_goal`, and exactly 30 `last_30_days` entries with `count` and `heat_level`.
- `days[29]` is treated as today.

## Compile

Use Arduino CLI with the ESP32-S3 board and the Huge APP partition. The default partition is too small for this sketch.

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app .
```

In Arduino IDE, select `ESP32S3 Dev Module` and `Huge APP (3MB No OTA/1MB SPIFFS)` for the partition scheme.
