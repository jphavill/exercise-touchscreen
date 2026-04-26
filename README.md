# Exercise Touchscreen (ESP-IDF)

This project now targets native ESP-IDF (no Arduino runtime).

## Supported ESP-IDF version

- Use ESP-IDF `v5.4.x` (recommended)

## Prerequisites

- ESP-IDF extension installed in VS Code
- ESP-IDF toolchain configured in the extension
- Target board connected over USB

## First-time setup

1. Open this folder in VS Code.
2. Use `ESP-IDF: Set Espressif Device Target` and select `esp32s3`.
3. Use `ESP-IDF: SDK Configuration Editor (Menuconfig)` and set:
   - `Exercise Touchscreen -> Wi-Fi SSID`
   - `Exercise Touchscreen -> Wi-Fi password`
   - `Exercise Touchscreen -> Dashboard API URL` (if needed)
   - `Exercise Touchscreen -> API timezone header` (if needed)

## Build / Flash / Monitor

- Build: `ESP-IDF: Build your project`
- Flash: `ESP-IDF: Flash your project`
- Monitor: `ESP-IDF: Monitor your device`

Equivalent CLI commands:

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build flash monitor
```
